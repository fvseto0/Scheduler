#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "memlayout.h"
#include "wmap.h"
//#include "vm.h"
//#include "stat.h"
//#include "user.h"
#include "x86.h"
//#include <stdint.h>
int 
sys_wmap(void)
{
	uint addr; // va to (maybe) use  
	int length; // size of mapping
	int flags; // mapping specifications 
	int fd; // file descripter if added 

	// check valid user input 
    if (argint(0, (int *)&addr) < 0 || argint(1, &length) < 0 ||
        argint(2, &flags) < 0 || argint(3, &fd) < 0) {
        return -1;  // if not valid 
    }
    // checks if valid addr 
    int move = 0; // var if the given addr should change

    if(addr%4096!=0) {
    	if(flags &MAP_FIXED)
    		return -1; 
    	int off=4096-addr%4096;
    	addr+=off;	
    }
	if(addr < 0x60000000 || addr > 0x80000000) {
		if(flags &MAP_FIXED)
		    return -1; 
		move = 1; 
	}
	
	// checks if address and length are in range
	if(addr+length > 0x80000000) {
		if(flags & MAP_FIXED)
			return -1; 
		move = 1; 
	}

	// Get the current process
	struct proc *curproc = myproc();
	// get the amount of pages that need to be allocated 
	int numPages = (length+4095)/4096;
//cprintf("pages: %d addr: %x\n",numPages,addr);
	// looks at all mappings already made checking for overlap in memory
	for(int i=0; i<curproc->wmapCount; i++) {
		if(move == 1) 
			break;
		if((curproc->wmappings[i]->addr <= addr &&
			curproc->wmappings[i]->addr+(curproc->wmappings[i]->length) >= addr) ||
			(curproc->wmappings[i]->addr <= (addr+length) &&
			curproc->wmappings[i]->addr+(curproc->wmappings[i]->length) >= (addr+length))) {
			// if there's overlap and map fixed, return 
			if(flags & MAP_FIXED) 
				return -1;
			move = 1; 
		}
	}
	// checks if the inserted addr is already mapped 
	if(move==0) { 	
//	cprintf("move==0");	 	
		curproc->wmappings[curproc->wmapCount] = (struct wmapInfo *)kalloc();
		curproc->wmappings[curproc->wmapCount]->addr = addr;
		curproc->wmappings[curproc->wmapCount]->length = length;
		curproc->wmappings[curproc->wmapCount]->flags = flags;
		curproc->wmappings[curproc->wmapCount]->fd = fd;
		curproc->wmapCount++;
	}else {
//	cprintf("move==1");
		 // look for new addr to map to
		 addr = 0x60000000; // changes addr to start of memory
		 int found = 0; // int checking if a spot in memory is available 
		 //int count = 0;
		 while(addr < 0x80000000) {
		 	if(curproc->wmapCount==0) {
		 		break;
		 	}
		 	for(int i=0; i<curproc->wmapCount; i++) {
		 		uint curmap= (uint)curproc->wmappings[i]->addr;
		 		if(!((uint)addr == curmap) && !(curproc->wmappings[i]->addr <= addr &&
		 			curproc->wmappings[i]->addr+(curproc->wmappings[i]->length) >= addr) &&
		 			!(curproc->wmappings[i]->addr <= (addr+length) &&
		 			curproc->wmappings[i]->addr+(curproc->wmappings[i]->length) >= (addr+length)) &&
		 			!(curproc->wmappings[i]->addr > (addr) &&
		 					 			curproc->wmappings[i]->addr< (addr+length))) {
		 			// if there's a spot in memory 
		 			found = 1;  
		 		}else {
		 			// if its found the spot is taken, break out of loop 
		 			found = 0;
		 			break;
		 		}
		 	}
		 	if(found == 1) {
		 		// if addr found map it and break out of loop
		 		curproc->wmappings[curproc->wmapCount] = (struct wmapInfo *)kalloc();
		 		curproc->wmappings[curproc->wmapCount]->addr = addr;
		 		curproc->wmappings[curproc->wmapCount]->length = length;
		 		curproc->wmappings[curproc->wmapCount]->flags = flags;
		 		curproc->wmappings[curproc->wmapCount]->fd = fd;
		 		curproc->wmapCount++; 
		 	//	cprintf("taken %x\n",addr);
		 		break;
		 	}
		 	addr+=0x1000; // increase addr	
		} 
	}
	//if(fd < 0) {
	for(int i=0; i<numPages; i++) {
		char *mem=kalloc(); 
		//if(mem==0){
			// free for loop to i 
	//	}
		// Access the page directory of the current process
		pde_t *pgdir = curproc->pgdir;
//		cprintf("before map: %x\n",addr);
	//	uint naddr=(uint)addr;
	//	void *newaddr = (void *)naddr; //(void*)(va+0x1000*i)
	//	pte_t *pte = (pte_t*)walkpgdir(curproc->pgdir, (void*)&addr+4096*i, 0);
	//	if(!(*pte &PTE_P))
		mappages(pgdir, (void *)(addr + 4096*i), 4096, V2P(mem), PTE_W|PTE_U);

	}
//	}
	cprintf("done\n",addr, length);
	return addr;
}

int sys_wunmap(void) {
	uint addr; 
	if (argint(0, (int*)&addr) < 0) {
		return -1; 
	}
	
//	if(addr==0){}
	struct proc *curproc = myproc();
	pde_t *pgdir = curproc->pgdir; 
	// addr thing wrong prob
	pte_t *pte = (pte_t*)walkpgdir(pgdir, (void*)&addr, 0);
	uint physical_address = PTE_ADDR(*pte);
	if(physical_address==0){}
	//	*pte=0;
	//get mapping-> get pde-> pte-> unmap eevrything PTE_U
		//int length = 0;
	// free from wmappings array in proc
	for(int i=0; i<curproc->wmapCount; i++) {
		if(curproc->wmappings[i]->addr == addr) {
			// free inside pte
			for(int j=0; j<curproc->wmappings[i]->length; j++) {
			//	kfree(P2V(physical_address+4096*i));
			} 
			// move everything at last addr to this addr (so when wmap again wont override current thing)
			if(i == curproc->wmapCount) {
				myproc()->wmappings[i]=0;
			}else {
				int lastIn = curproc->wmapCount-1;
				curproc->wmappings[i]->addr = curproc->wmappings[lastIn]->addr;
				curproc->wmappings[i]->length = curproc->wmappings[lastIn]->length;
				curproc->wmappings[i]->flags = curproc->wmappings[lastIn]->flags;
				curproc->wmappings[i]->fd = curproc->wmappings[lastIn]->fd;
			}
			
			myproc()->wmapCount--;
		//	kfree((char *)curproc->wmappings[i]);
		}
	}
	
	*pte = 0;
	return 0; 
}
uint sys_wremap(void) {
	uint oldaddr;
	int oldsize;
	int newsize;
	int flags; 
	

	if (argint(0, (int*)&oldaddr) < 0 || argint(1, &oldsize) < 0 ||
	        argint(2, &newsize) < 0 || argint(3, &flags) < 0) {
	        return 0;  // if not valid 
	}
	if(oldaddr==0){}
		if(oldsize==0){}
		if(newsize==0){}
		if(flags==0){}

	struct proc *curproc = myproc();
	pde_t *pgdir = myproc()->pgdir; 
	pte_t *pte = (pte_t*)walkpgdir(pgdir, (void*)&oldaddr, 0);
		
	if(oldsize > newsize) {
		// growing smaller so just free maps 
		if(pte != 0) {
			
					for(int i=0; i<myproc()->wmapCount; i++) {
						if(myproc()->wmappings[i]->addr == oldaddr) {
							//length = myproc()->wmappings[i]->length;
							// doesnt remove the map pages tho 
							myproc()->wmappings[i]->length=newsize;
							//myproc()->wmapCount--;
							break;
						}
					}
				
		}	
	}else {
		/*int found = 0; // checks if map has to move 
		for(int i=0; i<curproc->wmapCount; i++) {
			uint curmap= (uint)curproc->wmappings[i]->addr;
					 		if(!((uint)oldaddr == curmap) && !(curproc->wmappings[i]->addr <= oldaddr &&
					 			curproc->wmappings[i]->addr+(curproc->wmappings[i]->length) >= oldaddr) &&
					 			!(curproc->wmappings[i]->addr <= (oldaddr+newsize) &&
					 			curproc->wmappings[i]->addr+(curproc->wmappings[i]->length) >= (oldaddr+newsize)) &&
					 			!(curproc->wmappings[i]->addr > (oldaddr) &&
					 					 			curproc->wmappings[i]->addr< (oldaddr+newsize))) {
					 			// if there's a spot in memory 
					 			found = 1;  
					 		}else {
					 			// if its found the spot is taken, break out of loop 
					 			found = 0;
					 			break;
					 		}
		}*/



		int numPages = (newsize-oldsize+4095)/4096;
		//if(newsize%4096 != 0) 
			//numPages++; 
		if(numPages != 0) {
			 for(int i=0; i<numPages; i++) {
			 		char *mem=kalloc(); 

			 		mappages(pgdir, (void *)(oldaddr+oldsize + 4096*i), 4096, V2P(mem), PTE_W|PTE_U);
			 }
			 for(int i=0; i<myproc()->wmapCount; i++) {
			 						if(myproc()->wmappings[i]->addr == oldaddr) {
				curproc->wmappings[i]->length = newsize;
			 						}}
		}
	}
//	sys_wunmap(oldaddr);
//	sys_wmap(oldaddr, newsize, flags);
	return oldaddr; 
}
int sys_getpgdirinfo(void) {
	// arg stuff 
	struct pgdirinfo *pdinfo;
	if (argptr(0, (void*)&pdinfo, sizeof(*pdinfo)) < 0)
		return -1;
	// getting process
	struct proc *curproc = myproc();
	// getting pgdir
	pde_t* pgdir = curproc->pgdir; 
	// count for pages
	int pages = 0;
	//
	int c = 0;
	for(int i=0; i<32; i++) {
		// checks if current pde has an entry 
		if(!(pgdir[i] & PTE_P)) 
			continue; 
		cprintf("present %d\n",curproc->wmappings[i]->length);
		if((curproc->wmappings[i]->length) < 0) pages++;
		pte_t *pte = (pte_t*)P2V(PTE_ADDR(pgdir[i]));
		// go over all pte 
		for(int j=0; j<(curproc->wmappings[i]->length+4095)/4096; j++) {
			// checks is page is a user page + is present
		//	if(pte[j] & PTE_P && pte[j] & PTE_U) {
				// filling entrys in pgdirinfo 
				pdinfo->va[pages] = curproc->wmappings[i]->addr+c*4096;// PGADDR(i, j, 0);
				cprintf("pages; %d length %d\n",pdinfo->va[pages]+i*4096,(curproc->wmappings[i]->length+4095)/4096);
				pdinfo->pa[pages] = PTE_ADDR(pte[j]);
				pages++;
				c++;
		//	}
		}
		c=0;
	}
	pdinfo->n_upages = pages; 
		cprintf("pages; %d\n",pages);

	return 0; 
}
int sys_getwmapinfo(void) {
	// getting info 
	struct wmapinfo *wminfo; 
	if (argptr(0, (void*)&wminfo, sizeof(*wminfo)) < 0)
			return -1;
	struct proc *curproc = myproc(); 
	// get number of mmaps from proc var wmapCount
	wminfo->total_mmaps = curproc->wmapCount; 
	// add information to wmapinfo for every mapped process
	for(int i=0; i<curproc->wmapCount; i++) {
		wminfo->addr[i]=curproc->wmappings[i]->addr;
		wminfo->length[i]=curproc->wmappings[i]->length;
		int numPages=(wminfo->length[i]+4095)/4096;
		cprintf("num load paes %d \n", numPages);
	//	if(curproc->wmappings[i]->fd < 0)
		wminfo->n_loaded_pages[i]=numPages;
	}
	return 0;
}

