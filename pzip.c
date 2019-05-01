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

int num_threads;
char stored_zip[30000000];
int stored_size;
int controller = 0;
struct stored_data addr_list[24];

void *compress_file (void *slice);

int main (int argc, char *argv[]) {
    int i, j;
    int *k;
    pthread_t *threads;
	char *stored_data;
	struct stat sb;

    num_threads = atoi(argv[1]);

	for (int i = 2; i < (argc); i++){
		int fd = open(argv[i], O_RDONLY, S_IRUSR | S_IWUSR);
		fstat(fd, &sb);
		stored_data = mmap(NULL, (sb.st_size), PROT_READ, MAP_SHARED, fd, 0);
		addr_list[i-2].pointer = stored_data;
		addr_list[i-2].length = sb.st_size;
		stored_size += sb.st_size;
		close(fd);
	}

	printf("stored_size = %d\n", stored_size);

    threads = (pthread_t *) malloc (num_threads * sizeof (pthread_t));
    
    for (i = 0; i < num_threads; i++) {
        k = malloc (sizeof (int));
        *k = i;
        if (pthread_create (&threads[i], NULL, compress_file, k) != 0) {
            printf("Can't create thread %d\n", i);
            free (threads);
            exit (-1);
        }
    }
    
    // main thead waiting for other thread to complete
    for (i = 0; i < num_threads; i++){
        pthread_join (threads[i], NULL);
	}

	//unmap maps	
    
    fwrite(stored_zip, 1, sizeof(stored_zip), stdout);
	// fwrite(&count, sizeof(int), 1, stdout);
	// fputc(last, stdout);
    
    return 0;
}

void *compress_file (void *slice) {
    int l, k = *((int *) slice);
	int tempTotal = stored_size;
	int perThread = stored_size/num_threads;
    int start_pos_calc, start_pos_total = (perThread * k);
    int end_pos = (start_pos_total + (perThread - 1));

	struct stored_data tempFile;

	// Calc file to use and starting position
	if (k == 0) {
		tempFile = addr_list[0];
		start_pos_calc = 0;
	} else {
		for (l = 0; l < 25; l++){
			//printf("------------> T:%dL%d, spt: %d, adl[%d]: %d\n", k, l, start_pos_total, l, addr_list[l].length);
			if ((start_pos_total - addr_list[l].length) <= 0){
				tempFile = addr_list[l];
				start_pos_total = start_pos_total - tempFile.length;
				start_pos_calc = tempFile.length + start_pos_total;
				break;
			} else { start_pos_total = start_pos_total - addr_list[l].length; }
		}
	}

	if ((k + 1) == num_threads) // if stored_size % num_threads != 0, assign remaining info to last thread
		end_pos = tempFile.length;
    
    printf("thread %d, start_oval: %d, start_pos_calc: %d, adl[%d]\n", k, (perThread * k), start_pos_calc, l);

	//add logic for compress
	char buffer[100000];
	char last = tempFile.pointer[start_pos_calc];
	int count = 0;
	for (start_pos_calc; start_pos_calc <= end_pos; start_pos_calc++){
		//printf("thread %d, loop %d, char %c ----> ", k, start_pos_calc, tempFile.pointer[start_pos_calc]);
		if (start_pos_calc > tempFile.length){
			//printf("reached eof, next file is adl[%d]\n", l);
			sprintf(buffer, "%s%d%c", buffer, count, last);
			tempFile = addr_list[l+1]; //maybe right?
			start_pos_calc = 1;
			last = tempFile.pointer[0];
			count = 0;
		} if (perThread == 0){
			//printf("thread %d done, break\n", k);
			sprintf(buffer, "%s%d%c", buffer, count, last);
			break;
		} if (tempFile.pointer[start_pos_calc] == last){
			//printf("incremented\n");
			count++;
		} if (tempFile.pointer[start_pos_calc] != last) {
			//printf("printed %d%c\n", count, last);
			sprintf(buffer, "%s%d%c", buffer, count, last);
			last = tempFile.pointer[start_pos_calc];
			count = 1;
		}
		perThread--;
		//another flag that im missing?
	}

	//USE FWRITE/FPUT TO WRITE TO BUFFER (OR WHATEVER)

	//catch straggler
	if(k != num_threads) sprintf(buffer, "%s%d%c", buffer, count, last);
    
    while(1){ // wait for turn to update stored_zip
        if (controller == k){
            //printf("\n\nthread: %d, stored: %s, buffer: %s\n\n", k, stored_zip, buffer);
			sprintf(stored_zip, "%s%s", stored_zip, buffer);
            controller = k+1;
            break;
        }
    } // gets stuck on the last thread check for loop exit condition
    
    return 0;
}
