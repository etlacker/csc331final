// eric lacker
// ID 870124
// csc331 assignment 4


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
    }
    
    // main thead waiting for other thread to complete
    for (i = 0; i < num_threads; i++){
        pthread_join (threads[i], NULL);
	}

    return 0;
}

void *compress_file (void *slice) {
    int k = *((int *) slice);
	int l = 0;
	int perThread = (stored_size/num_threads);
    int start_pos_calc;
	int start_pos_total = (perThread * k);
	
	struct stored_zipped zipped_list[perThread*2];
	int zip_count = -1;

	struct stored_data tempFile;
	// Calc file to use and starting position
	if (k == 0) {
		tempFile = addr_list[0];
		start_pos_calc = 0;
	} else {
		for (l = 0; l < 24; l++){
			if ((start_pos_total - addr_list[l].length) <= 0){
				tempFile = addr_list[l];
				start_pos_total = start_pos_total - tempFile.length;
				start_pos_calc = tempFile.length + start_pos_total;
				break;
			} else { start_pos_total = start_pos_total - addr_list[l].length; }
		}
	}

	if ((k + 1) == num_threads) // if stored_size % num_threads != 0, assign remaining info to last thread
		perThread += 10000;

	char last = tempFile.pointer[start_pos_calc];
	int count = 0;
	while (perThread >= 0){
		if (start_pos_calc > tempFile.length){
			if (addr_list[l+1].length > 0){
				l++;
				tempFile = addr_list[l]; 
				start_pos_calc = 1;
				last = tempFile.pointer[0];
				count = 1;
				continue;
			} else { break; }
		} if (perThread == 0){
			zip_count++;
			zipped_list[zip_count].count = count;
			zipped_list[zip_count].chara = last;
			break;
		} if (tempFile.pointer[start_pos_calc] == last){
			count++;
		} if (tempFile.pointer[start_pos_calc] != last) {
			zip_count++;
			zipped_list[zip_count].count = count;
			zipped_list[zip_count].chara = last;
			last = tempFile.pointer[start_pos_calc];
			count = 1;
		}
		perThread--;
		start_pos_calc++;
	}

    while(1){ // wait for turn to update stored_zip
        if (controller == k){
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
