#include "emufs.h" 
/**************************** File system helper functions ****************************/

//	Function to encrypt a block of data 
int encrypt(char* input, char* encrypted)
{
	for(int i=0; i<BLOCKSIZE; i++)
	{
		encrypted[i] = ~input[i];
	}
	return 1;
}

//	Function to decrypt a block of data 
int decrypt(char* input, char* decrypted)
{
	for(int i=0; i<BLOCKSIZE; i++)
	{
		decrypted[i] = ~input[i];
	}
	return 1;
}

//	The following helper functions are used to read and write 
//	superblock and metadata block. 
//	Change the function definitions with required arguments
struct superblock_t* readSuperblock(int fd)
{
	/*
		* Read 0th block from the device into a blocksize buffer
		* Create superblock_t variable and fill it using reader buffer
		* Return the superblock_t variable
	*/
	char tempBuf[BLOCKSIZE];
	if(readblock(fd, 0, tempBuf) < 0)
		return NULL;
	struct superblock_t* temp = (struct superblock_t*) malloc (sizeof(struct superblock_t));
	if(temp == NULL) {
		printf("Error reading superblock into memory\n");
		return NULL;
	}
	memcpy(temp, tempBuf, sizeof(struct superblock_t));
	return temp;
}

int writeSuperblock(int fd, struct superblock_t* superblock)
{
	/*
		* Write the superblock into the buffer
		* Write back the buffer into block 0
	*/
	char tempBuf[BLOCKSIZE];
	memcpy(tempBuf, (char *)superblock, sizeof(struct superblock_t));
	return writeblock(fd, 0, tempBuf);
}

struct metadata_t* readMetadata(int fd, int fs_number)
{
	// Same as readSuperBlock(), but it is stored on block 1
	// Need to decrypt if emufs-encrypted is used  
	char tempBuf[BLOCKSIZE], readBuf[BLOCKSIZE];
	if(readblock(fd, 1, tempBuf) < 0)
		return NULL;

	if(fs_number == 1)	decrypt(tempBuf, readBuf);
	else				memcpy(readBuf, tempBuf, sizeof(tempBuf));

	struct metadata_t* temp = (struct metadata_t*) malloc (sizeof(struct metadata_t));
	if(temp == NULL)
		return NULL;

	memcpy(temp, readBuf, sizeof(struct metadata_t));
	return temp;
}

int writeMetadata(int fd, int fs_number, struct metadata_t* metadata)
{
	// Same as writeSuperblock(), but it is stored on block 1
	// Need to decrypt/encrypt if emufs-encrypted is used  
	char tempBuf[BLOCKSIZE];
	memcpy(tempBuf, (char *)metadata, sizeof(struct metadata_t));
	if(fs_number == 1)	encrypt(tempBuf, tempBuf);
	return writeblock(fd, 1, tempBuf);
}

/**************************** File system API ****************************/

int create_file_system(struct mount_t *mount_point, int fs_number)
{
	/*
	   	* Read the superblock.
	    * Set file system number on superblock
		* Clear the bitmaps.  values on the bitmap will be either '0', or '1', or'x'. 
		* Create metadata block in disk
		* Write superblock and metadata block back to disk.

		* Return value: -1,		error
						 1, 	success
	*/
	if(mount_point == NULL)
		return -1;
	int device_fd = mount_point->device_fd, i;
	
	struct superblock_t *sprblock = readSuperblock(device_fd);
	if(sprblock == NULL)  return -1;

	sprblock->fs_number = fs_number;

	// All blocks within the disk have not been allocated
	for(i = 1; i < sprblock->disk_size; i++)  sprblock->bitmap[i] = '0';
	// Blocks beyond disk size are not available
	for(i = sprblock->disk_size; i < MAX_BLOCKS; i++) sprblock->bitmap[i] = 'x';
	// The block corresponding to superblock has been allocated
	sprblock->bitmap[0] = '1';

	struct metadata_t *metadata = (struct metadata_t*) malloc (sizeof(struct metadata_t));
	if(metadata == NULL){
		free(sprblock);
		printf("Error creating metadata\n");
		return -1;
	}

	for(i = 0; i < MAX_FILES; i++) {
		metadata->inodes[i].status = 0;
		metadata->inodes[i].blocks[0] = -1;
		metadata->inodes[i].blocks[1] = -1;
		metadata->inodes[i].blocks[2] = -1;
		metadata->inodes[i].blocks[3] = -1;
	}

	sprblock->bitmap[1] = '1';
	if(writeSuperblock(mount_point->device_fd, sprblock) < 0 || writeMetadata(mount_point->device_fd, sprblock->fs_number, metadata) < 0) {
		free(sprblock);
		free(metadata);
		printf("Error writing superblock and/or metadata\n");
		return -1;
	}

	free(sprblock);
	free(metadata);
	mount_point->fs_number = fs_number;
	return 1;
}


struct file_t* eopen(struct mount_t* mount_point, char* filename)
{
	/* 
		* If file exist, get the inode number. inode number is the index of inode in the metadata.
		* If file does not exist, 
			* find free inode.
			* allocate the free inode as USED
			* if free id not found, print the error and return -1
		* Create the file hander (struct file_t)
		* Initialize offset in the file hander
		* Return file handler.

		* Return NULL on error.
	*/
	int i, device_fd = mount_point->device_fd, index = MAX_FILES;
	struct superblock_t *superblock = readSuperblock(device_fd);
	struct metadata_t *metadata = readMetadata(device_fd, superblock->fs_number);
	if(metadata == NULL)
		return NULL;

	struct file_t *file = (struct file_t*) malloc (sizeof(struct file_t));
	
	for(i = 0; i < MAX_FILES; i++) {
		if(metadata->inodes[i].status == 0) {
			continue;
		}
		if(strcmp(metadata->inodes[i].name, filename) == 0) {
			index = i;
			break;
		}
	}

	if(index == MAX_FILES) {
		for(index = 0; index < MAX_FILES; index++)
			if(metadata->inodes[index].status == 0)	break;
		if(index == MAX_FILES) {
			printf("Max capacity of inodes reached\n");
			free(file);
			file = NULL;
		}
		else {
			metadata->inodes[index].status = 1;
			strcpy(metadata->inodes[index].name, filename);
			metadata->inodes[index].file_size = 0;
			file->offset = 0;
			file->inode_number = index;
			file->mount_point = mount_point;
			writeMetadata(device_fd, superblock->fs_number, metadata);
			printf("New Inode %d\n", index);
		}
	} else {
		file->offset = 0;
		file->inode_number = index;
		file->mount_point = mount_point;
	}
	free(metadata);
	return file;
}

int ewrite(struct file_t* file, char* data, int size)
{
	// You do not need to implement partial writes in case file exceeds 4 blocks
	// or no free block is available in the disk. 

	// Return value: 	-1,  error
	//					Number of bytes written
	if(size % BLOCKSIZE != 0) return -1;
	if(file->offset + size > 4*BLOCKSIZE)	return -1;

	int i, j, index, fd = file->mount_point->device_fd;
	struct superblock_t *superblock = readSuperblock(fd);
	struct metadata_t *metadata = readMetadata(fd, superblock->fs_number);
	
	for(i = file->offset; i < file->offset+size; i += BLOCKSIZE){
		index = i / BLOCKSIZE;
		if(metadata->inodes[file->inode_number].blocks[index] == -1) {
			for(j = 2; j < MAX_BLOCKS; j++)
				if(superblock->bitmap[j] == '0')  break;
			if(j == MAX_BLOCKS){
				free(metadata);
				free(superblock);
				return -1;
			}

			superblock->bitmap[j] = '1';
			metadata->inodes[file->inode_number].blocks[index] = j;
			metadata->inodes[file->inode_number].modtime = time(NULL);
			metadata->inodes[file->inode_number].file_size += BLOCKSIZE;
			
			if(writeSuperblock(fd, superblock) < 0 || writeMetadata(fd, superblock->fs_number, metadata) < 0) {
				free(metadata);
				free(superblock);
				printf("Failed to write\n");
				return -1;
			}
		}
		if(writeblock(fd, metadata->inodes[file->inode_number].blocks[index], data) < 0) {
			free(metadata);
			free(superblock);
			return -1;
		}
		data += BLOCKSIZE;
	}

	file->offset += size;
	free(metadata);
	free(superblock);
	return size;
}

int eread(struct file_t* file, char* data, int size)
{
	// NO partial READS.

	// Return value: 	-1,  error
	//					Number of bytes read
	if(size % BLOCKSIZE != 0) return -1;
	if(file->offset + size > 4*BLOCKSIZE)	return -1;

	int i, index, fd = file->mount_point->device_fd;
	struct superblock_t *superblock = readSuperblock(fd);
	struct metadata_t *metadata = readMetadata(fd, superblock->fs_number);

	for(i = file->offset; i < file->offset+size; i += BLOCKSIZE){
		index = i / BLOCKSIZE;
/*		if(metadata->inodes[file->inode_number].blocks[index] == -1) {
			size = i - file->offset;
			break;
		}
*/		if(readblock(fd, metadata->inodes[file->inode_number].blocks[index], data) < 0) {
			free(metadata);
			free(superblock);
			return -1;
		}
		data += BLOCKSIZE;
	}
		
	file->offset += size;
	//printf("%d %d %s\n",file->offset,index,data);
	free(metadata);
	free(superblock);
	return size;
}

void eclose(struct file_t* file)
{
	// free the memory allocated for the file handler 'file'
	free(file);
}

int eseek(struct file_t *file, int offset)
{
	// Change the offset in file hanlder 'file'
	if(offset % BLOCKSIZE != 0)	return -1;
	file->offset = offset;
	return 1;
}

void fsdump(struct mount_t* mount_point)
{
	int i, size;

	printf("\n[%s] fsdump \n", mount_point->device_name);
	printf("%-10s %6s \t[%s] \t%s\n", "  NAME", "SIZE", "BLOCKS", "LAST MODIFIED");
	
	struct superblock_t *superblock = readSuperblock(mount_point->device_fd);
	struct metadata_t *metadata = readMetadata(mount_point->device_fd, superblock->fs_number);
	for(i = 0; i < MAX_FILES; i++) {
		struct inode_t inode = metadata->inodes[i];
		if(inode.status != 1)	continue;
		for(size = 0; inode.blocks[size] != -1 && size < 4; size++);
		printf("%-10s %6d \t[%d %d %d %d] \t%s", 
			inode.name, 
			size * BLOCKSIZE,
			inode.blocks[0],
			inode.blocks[1],
			inode.blocks[2],
			inode.blocks[3],
			asctime(localtime(&(inode.modtime)))
			);
	}

}
