#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>
#include <unistd.h> //for getpagesize
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Link.h"
#include "LoaderInternal.h"
#define ALIGN_DOWN(base, size) ((base) & -((__typeof__(base))(size)))
#define ALIGN_UP(base, size) ALIGN_DOWN((base) + (size)-1, (size))
const int MAXPHDR=24;

static const char *sys_path[] = {
    "/usr/lib/x86_64-linux-gnu/",
    "/lib/x86_64-linux-gnu/",
    ""
};

static const char *fake_so[] = {
    "libc.so.6",
    "ld-linux.so.2",
    ""
};

static void setup_hash(LinkMap *l)
{
    uint32_t *hash;

    /* borrowed from dl-lookup.c:_dl_setup_hash */
    Elf32_Word *hash32 = (Elf32_Word *)l->dynInfo[DT_GNU_HASH_NEW]->d_un.d_ptr;
    l->l_nbuckets = *hash32++;
    Elf32_Word symbias = *hash32++;
    Elf32_Word bitmask_nwords = *hash32++;

    l->l_gnu_bitmask_idxbits = bitmask_nwords - 1;
    l->l_gnu_shift = *hash32++;

    l->l_gnu_bitmask = (Elf64_Addr *)hash32;
    hash32 += 64 / 32 * bitmask_nwords;

    l->l_gnu_buckets = hash32;
    hash32 += l->l_nbuckets;
    l->l_gnu_chain_zero = hash32 - symbias;
}

static void fill_info(LinkMap *lib)
{
    Elf64_Dyn *dyn = lib->dyn;
    Elf64_Dyn **dyn_info = lib->dynInfo;

    while (dyn->d_tag != DT_NULL)
    {
        if ((Elf64_Xword)dyn->d_tag < DT_NUM)
            dyn_info[dyn->d_tag] = dyn;
        else if ((Elf64_Xword)dyn->d_tag == DT_RELACOUNT)
            dyn_info[DT_RELACOUNT_NEW] = dyn;
        else if ((Elf64_Xword)dyn->d_tag == DT_GNU_HASH)
            dyn_info[DT_GNU_HASH_NEW] = dyn;
        ++dyn;
    }
    #define rebase(tag)                             \
        do                                          \
        {                                           \
            if (dyn_info[tag])                          \
                dyn_info[tag]->d_un.d_ptr += lib->addr; \
        } while (0)
    rebase(DT_SYMTAB);
    rebase(DT_STRTAB);
    rebase(DT_RELA);
    rebase(DT_JMPREL);
    rebase(DT_GNU_HASH_NEW); //DT_GNU_HASH
    rebase(DT_PLTGOT);
    rebase(DT_INIT);
    rebase(DT_INIT_ARRAY);
}

static int prot_from_phdr(const Elf64_Phdr* phdr)
{
    int prot=0;
    if (phdr->p_flags & PF_R) prot|=PROT_READ;
    if (phdr->p_flags & PF_W) prot|=PROT_WRITE;
    if (phdr->p_flags & PF_X) prot|=PROT_EXEC;
    return prot;
}

void *MapLibrary(const char *libpath)
{
	LinkMap* lib=(LinkMap*)malloc(sizeof(LinkMap));

    	int fd=open(libpath,O_RDONLY);
	Elf64_Ehdr ehdr;
	pread(fd,&ehdr,sizeof(Elf64_Ehdr),0);
	
	Elf64_Phdr phdr[MAXPHDR];
	
	size_t size_phdr=sizeof(Elf64_Phdr);
	size_t size_page=getpagesize();
	int flag=0;
	pread(fd,&phdr,size_phdr*ehdr.e_phnum,ehdr.e_phoff);
	
	size_t i=0;
    	while (i<ehdr.e_phnum && phdr[i].p_type!=PT_LOAD) i++;
    	
    	Elf64_Phdr* first_segment = &phdr[i];
    	Elf64_Phdr* last_segment = &phdr[ehdr.e_phnum-1];
    	while (last_segment>first_segment && last_segment->p_type!=PT_LOAD) last_segment--;
    	size_t span=last_segment->p_vaddr+last_segment->p_memsz-first_segment->p_vaddr;
    	
    	uintptr_t start_addr=(uintptr_t)mmap(NULL,ALIGN_UP(span,size_page),prot_from_phdr(first_segment),MAP_FILE|MAP_PRIVATE,fd,ALIGN_DOWN(first_segment->p_offset,size_page));
    	//(void*)ALIGN_DOWN(first_segment->p_vaddr,size_page)
    	const Elf64_Addr load_bias=start_addr-ALIGN_DOWN(first_segment->p_vaddr,size_page);
    	//Elf64_Addr last_end=first_segment->p_vaddr+load_bias+first_segment->p_memsz;
    	lib->addr=(uint64_t)start_addr;
    	
    	for(Elf64_Phdr* ph=first_segment+1;ph<=last_segment;++ph)
    	{
    		if(ph->p_type==PT_LOAD)
    		{
            		Elf64_Addr start=ALIGN_DOWN(ph->p_vaddr+load_bias,size_page);
            		Elf64_Addr end=ALIGN_UP(ph->p_vaddr+load_bias+ph->p_memsz,size_page);
            		mmap((void*)start,end-start,prot_from_phdr(ph),MAP_FILE|MAP_PRIVATE|MAP_FIXED,fd,ALIGN_DOWN(ph->p_offset,size_page));
    		}
    	}
    	
    	Elf64_Dyn* dynamic;
    	for (i=0;i<ehdr.e_phnum;++i)
    	{
        	if(phdr[i].p_type==PT_DYNAMIC) dynamic=(Elf64_Dyn*)(load_bias+phdr[i].p_vaddr);
        }
    	lib->dyn=dynamic;
	fill_info(lib);
     	setup_hash(lib);
     	return lib;
}
