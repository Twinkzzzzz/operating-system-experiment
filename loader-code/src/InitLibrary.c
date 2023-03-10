#include <stdlib.h>
#include <stdio.h>
#include <elf.h>
#include <stdint.h>

#include "Link.h"
#include "LoaderInternal.h"

void InitLibrary(LinkMap *lib)
{
	const int MAX_INIT=20;
    	Elf64_Dyn* dynamic=lib->dyn;
    	Elf64_Rela* rela;
    	Elf64_Sword tag;
    	int size_section,size_entry;
    	int localnum;
    	void (*initfunc)();
    	void** initlist;
    	int funcnum;
    	
    	while (dynamic->d_tag!=DT_NULL)
    	{
    		dynamic++;
    		tag=dynamic->d_tag;
    		if(tag==DT_RELA)
    		{
    			rela=(Elf64_Rela*)(dynamic->d_un.d_val);
		}
		else if(tag==DT_RELACOUNT)
		{
			localnum=(int)(dynamic->d_un.d_val);
		}
		else if(tag==DT_INIT)
		{
			initfunc=(void*)(dynamic->d_un.d_val);
		}
		else if(tag==DT_INIT_ARRAY)
		{
			initlist=(void*)(dynamic->d_un.d_val);
		}
		else if(tag==DT_INIT_ARRAYSZ)
		{
			funcnum=(int)(dynamic->d_un.d_val);
		}
    	}
    	for(int i=0;i<localnum;++i)
    	{
    		uint64_t* addr;
    		addr=(uint64_t*)(lib->addr+rela->r_offset);
    		*addr=(uint64_t)(lib->addr+(uint64_t)rela->r_addend);
    		//printf("%d\n",(int)(rela->r_addend));
    		rela++;
    	}
    	initfunc();
    	for(int i=0;i<funcnum/8;++i) //64->8 bytes
    	{
    		initfunc=(void*)(*initlist);
    		initfunc();
    		initlist++;
    	}
	return;
	
}
