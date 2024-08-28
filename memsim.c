#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef struct {
        int pageNo;
        int modified;
} page;

typedef struct {
	page page;
	int usage_data;
} pte;
enum    repl { rng, fifo, lru, clock};
int     createMMU(int);
int     checkInMemory(int, int, enum repl);
int     allocateFrame(int, int, enum repl);
int 	findMinUsage();
int 	findClockVictim();
page    selectVictim(int, int, enum repl);
const   int pageoffset = 12;            /* Page size is fixed to 4 KB */
int     numFrames;
int clock_hand = 0;
pte* page_table;

/* Creates the page table structure to record memory allocation */
int createMMU (int frames)
{

        // to do
		page_table = malloc(sizeof(pte)*frames);
		if (page_table == NULL) return -1;
		// Set all usage data to -1  (flag for unallocated)
		for (int i = 0; i < frames; i++) {
			page_table[i].usage_data = -1;
		}
        return 0;
}

/* Checks for residency: returns frame no or -1 if not found */
int checkInMemory(int page_number, int event_number, enum repl replace)
{
        int     result = -1;

        // Loop through page table to find the requested page number
		for (int i = 0; i < numFrames; i++) {
			if (page_table[i].usage_data == -1) continue;
			if (page_table[i].page.pageNo == page_number) {
				result = i;
				// For lru and clock, update the latest access time
				switch (replace)
				{
				case lru:
					page_table[i].usage_data = event_number;
					break;
				case clock:
					page_table[i].usage_data = 1;
				default:
					break;
				}
				break;
			}
		}
        return result;
}

/* allocate page to the next free frame and record where it put it */
int allocateFrame(int page_number, int event_number, enum repl replace)
{
        // to do
		int i = 0;
		// Find first unassigned page
		while (page_table[i].usage_data != -1) i++;

		// Add the new page, (unmodified), and set the initial usage value depending on replacement policy
		page_table[i].page.pageNo = page_number;
		page_table[i].page.modified = 0;
		switch (replace) {
			case lru:
			case fifo:
				page_table[i].usage_data = event_number;
				break;
			case clock:
			case rng:
				page_table[i].usage_data = 1;
				break;
		}
        return i;
}

// Returns the index of the PTE with the lowest usage data value (lru and fifo)
int findMinUsage() 
{
	int min_usage = page_table[0].usage_data;
	int min_usage_index = 0;
	for (int i = 1; i < numFrames; i++) {
		if (page_table[i].usage_data < min_usage) {
			min_usage = page_table[i].usage_data;
			min_usage_index = i;
		}
	}
	return min_usage_index;
}

// Returns the index of the first entry with a usage value of 0 (clock)
int findClockVictim() 
{
	// int clock_hand = 0;
	while(page_table[clock_hand].usage_data != 0) {
		page_table[clock_hand].usage_data = 0;
		clock_hand++;
		if (clock_hand >= numFrames) clock_hand %= numFrames;
		
	}
	return clock_hand;
}

/* Selects a victim for eviction/discard according to the replacement algorithm,  returns chosen frame_no  */
page selectVictim(int page_number, int event_number, enum repl  mode )
{
        page victim;
		// Get the index of the victim page, depending on the replacement policy
		int victim_frame_no = 0;
		
		switch (mode) {
			case lru:
			case fifo:
				victim_frame_no = findMinUsage();
				break;
			case rng:
				victim_frame_no = rand() % numFrames;
				break;
			case clock:
				victim_frame_no = findClockVictim();
				// clock_hand;
				clock_hand++;
				clock_hand %= numFrames;
				break;
		}
		//printf("victim page index: %d\n", victim_frame_no);
		// Copy victim frame data out
		victim.pageNo = page_table[victim_frame_no].page.pageNo;
		victim.modified = page_table[victim_frame_no].page.modified;

		// Swap new page in to the victim frame
		page_table[victim_frame_no].page.pageNo = page_number;
		page_table[victim_frame_no].page.modified = 0;
		switch (mode) {
			case lru:
			case fifo:
				page_table[victim_frame_no].usage_data = event_number;
				break;
			case clock:
			case rng:
				page_table[victim_frame_no].usage_data = 1;
				break;
		}
        return (victim);
}

		
int main(int argc, char *argv[])
{
  
	char	*tracename;
	int	page_number,frame_no, done ;
	int	do_line, i;
	int	no_events, disk_writes, disk_reads;
	int     debugmode;
 	enum	repl  replace;
	int	allocated=0; 
	int	victim_page;
        unsigned address;
    	char 	rw;
	page	Pvictim;
	FILE	*trace;

	// Check for correct number of arguments
    if (argc < 5) {
        printf( "Usage: ./memsim inputfile numberframes replacementmode debugmode \n");
        exit ( -1);
	} else {
        tracename = argv[1];	
		trace = fopen( tracename, "r");
		// Check for valid file
		if (trace == NULL ) {
            printf( "Cannot open trace file %s \n", tracename);
            exit (-1);
		}
		numFrames = atoi(argv[2]);
		// Check for valid number of frames
        if (numFrames < 1) {
            printf( "Frame number must be at least 1\n");
            exit (-1);
        }
		// Check for valid replacement policy
        if (strcmp(argv[3], "lru\0") == 0) {
            replace = lru;
		} else if (strcmp(argv[3], "rand\0") == 0) {
	     replace = rng;
		} else if (strcmp(argv[3], "clock\0") == 0) {
            replace = clock;		 
		} else if (strcmp(argv[3], "fifo\0") == 0) {
            replace = fifo;		 
		} else {
            printf( "Replacement algorithm must be rand/fifo/lru/clock  \n");
            exit (-1);
	  	}
		// Check for valid logging level
        if (strcmp(argv[4], "quiet\0") == 0){
            debugmode = 0;
		} else if (strcmp(argv[4], "debug\0") == 0) {
            debugmode = 1;
		} else {
            printf( "Replacement algorithm must be quiet/debug  \n");
            exit (-1);
	  	}
	}
	
	// Create MMU
	done = createMMU(numFrames);
	if (done == -1) {
		printf( "Cannot create MMU" );
		exit(-1);
    }

	no_events = 0;
	disk_writes = 0;
	disk_reads = 0;

    do_line = fscanf(trace,"%x %c",&address,&rw);
	// Read lines of input file and simulate actions
	while (do_line == 2)
	{
		page_number =  address >> pageoffset;
		frame_no = checkInMemory(page_number, no_events, replace) ;    /* ask for physical address */
		// printf("clock hand: %d\n", clock_hand);
		if ( frame_no == -1 )
		{
		  	disk_reads++ ;			/* Page fault, need to load it into memory */
		  	if (debugmode) {
		      printf( "Page fault %8x \n", page_number);
			}
		  	if (allocated < numFrames) {  			/* allocate it to an empty frame */
                frame_no = allocateFrame(page_number, no_events, replace);
		     	allocated++;
            } else {
		      	Pvictim = selectVictim(page_number, no_events, replace);   /* returns page number of the victim  */
		      	frame_no = checkInMemory(page_number, no_events, replace);    /* find out the frame the new page is in */
		   		if (Pvictim.modified) {          /* need to know victim page and modified  */
                    disk_writes++;			    
                    if (debugmode) printf( "Disk write %8x \n", Pvictim.pageNo);
		      	}
		   		else {
                    if (debugmode) printf( "Discard    %8x \n", Pvictim.pageNo);
				}
		   	}
		}
		if ( rw == 'R') {
		    if (debugmode) printf( "reading    %8x \n\n", page_number);
		} else if ( rw == 'W'){
		    // mark page in page table as written - modified
			page_table[frame_no].page.modified = 1;
		    if (debugmode) printf( "writing    %8x \n\n", page_number);
		} else {
		      printf( "Badly formatted file. Error on line %d\n", no_events+1); 
		      exit (-1);
		}

		no_events++;
        do_line = fscanf(trace,"%x %c",&address,&rw);
	}

	printf( "total memory frames:  %d\n", numFrames);
	printf( "events in trace:      %d\n", no_events);
	printf( "total disk reads:     %d\n", disk_reads);
	printf( "total disk writes:    %d\n", disk_writes);
	printf( "page fault rate:      %.4f\n", (float) disk_reads/no_events);
}
				
