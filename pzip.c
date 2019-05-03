#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

struct stored_data{
	char * pointer;
	int length;
};

struct stored_zipped{
	int count;
	char chara;
};

int num_threads;
int stored_size;
int controller = 0;
struct stored_data addr_list[24];

void *compress_file (void *slice);

int main (int argc, char *argv[]) {
    int *k, i;
    pthread_t *threads;
	char *stored_data;
	struct stat sb;

    num_threads = atoi(argv[1]);

	for (i = 2; i < (argc); i++){
		int fd = open(argv[i], O_RDONLY, S_IRUSR | S_IWUSR);
		fstat(fd, &sb);
		stored_data = mmap(NULL, (sb.st_size), PROT_READ, MAP_SHARED, fd, 0);
		addr_list[i-2].pointer = stored_data;
		addr_list[i-2].length = sb.st_size;
		stored_size += sb.st_size;
		close(fd);
		printf("\nloop %d\n\n", i);
	}

    threads = (pthread_t *) malloc (num_threads * sizeof (pthread_t));
    
    for (i = 0; i < num_threads; i++) {
        k = malloc (sizeof (int));
        *k = i;
        if (pthread_create (&threads[i], NULL, compress_file, k) != 0) {
            printf("Can't create thread %d\n", i);
            free (threads);
            exit (-1);
        }
	printf("\ncreated thread %d\n\n", i);
    }
    
    // main thead waiting for other thread to complete
    for (i = 0; i < num_threads; i++){
        pthread_join (threads[i], NULL);
	}

	//unmap maps	
    
    return 0;
}

void *compress_file (void *slice) {
    int k = *((int *) slice);
	printf("In thread %d\n", k);
	int l = 0;
	int perThread = stored_size/num_threads;
    int start_pos_calc, start_pos_total = (perThread * k);

	struct stored_zipped zipped_list[perThread*2];
	int zip_count = -1;
	printf("\n k: %d, perThread: %d\n\n", k, perThread);
	struct stored_data tempFile;
	// Calc file to use and starting position
	if (k == 0) {
		tempFile = addr_list[0];
		start_pos_calc = 0;
	} else {
		for (l = 0; l < 24; l++){
			printf("------------> T:%dL%d, spt: %d, adl[%d]: %d\n", k, l, start_pos_total, l, addr_list[l].length);
			if ((start_pos_total - addr_list[l].length) <= 0){
				tempFile = addr_list[l];
				start_pos_total = start_pos_total - tempFile.length;
				start_pos_calc = tempFile.length + start_pos_total;
				break;
			} else { start_pos_total = start_pos_total - addr_list[l].length; }
		}
	}
	
	printf("calc: %d, total: %d\n", start_pos_calc, start_pos_total);

	printf("thread %d, start_oval: %d, start_pos_calc: %d, adl[%d]\n", k, (perThread * k), start_pos_calc, l);

	if ((k + 1) == num_threads) // if stored_size % num_threads != 0, assign remaining info to last thread
		perThread += 10000;

	//add logic for compress
	//char buffer[100000];
	char last = tempFile.pointer[start_pos_calc];
	int count = 0;
	//start_pos_calc++;
	while (perThread >= 0){
		//printf("thread %d, loop %d, char %c ----> ", k, start_pos_calc, tempFile.pointer[start_pos_calc]);
		if (start_pos_calc > tempFile.length){
			//printf("reached eof, next file is adl[%d]: current \"%d%c\"\n", l+1, count, last);
			if (addr_list[l+1].length > 0){
				l++;
				tempFile = addr_list[l]; 
				start_pos_calc = 1;
				last = tempFile.pointer[0];
				count = 1;
				continue;
			} else { break; }
		} if (perThread == 0){
			//printf("thread %d done, break. curr: '%d%c'\n", k, count, last);
			//sprintf(buffer, "%s%d%c", buffer, count, last);
			zip_count++;
			zipped_list[zip_count].count = count;
			zipped_list[zip_count].chara = last;
			break;
		} if (tempFile.pointer[start_pos_calc] == last){
			//printf("incremented\n");
			count++;
		} if (tempFile.pointer[start_pos_calc] != last) {
			//printf("printed %d%c\n", count, last);
			//sprintf(buffer, "%s%d%c", buffer, count, last);
			zip_count++;
			zipped_list[zip_count].count = count;
			zipped_list[zip_count].chara = last;
			last = tempFile.pointer[start_pos_calc];
			count = 1;
		}
		perThread--;
		start_pos_calc++;
	}

	//mutex lock???
    
    while(1){ // wait for turn to update stored_zip
        if (controller == k){
            //printf("\n\nthread: %d, stored: %s, buffer: %s\n\n", k, stored_zip, buffer);
			//sprintf(stored_zip, "%s%s", stored_zip, buffer);
			int zip_print = 0;
			while (zip_print <= zip_count){
				fwrite(&zipped_list[zip_print].count, 4, 1, stdout);
				fwrite(&zipped_list[zip_print].chara, 1, 1, stdout);
				zip_print++;
			}
            controller = k+1;
            break;
        }
    }
    
    return 0;
}
