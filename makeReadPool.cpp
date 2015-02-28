#include "makeReadPool.h"
#include <cassert>
#include <cstdlib>
void realloc(sglPoolb &ret,int l){
  ret.m =l;
  kroundup32(ret.m);
  bam1_t **tmp = new bam1_t*[ret.m];
  memcpy(tmp,ret.reads,sizeof(bam1_t**)*ret.l);
  delete [] ret.reads;
  ret.reads = tmp;
  
  int *tmpI = new int[ret.m];
  memcpy(tmpI,ret.first,sizeof(int)*ret.l);
  delete [] ret.first;
  ret.first = tmpI;
  
    
  tmpI = new int[ret.m];
  memcpy(tmpI,ret.last,sizeof(int)*ret.l);
  delete [] ret.last;
  ret.last = tmpI;
  
}


void read_reads_usingStopb(htsFile *fp,int nReads,int &isEof,sglPoolb &ret,int refToRead,iter_t *it,int stop,int &rdObjEof,int &rdObjRegionDone,bam_hdr_t *hdr) {
#if 0
  fprintf(stderr,"[%s]\n",__FUNCTION__);
#endif 

  //if should never be in this function if we shouldnt read from the file.
  assert(rdObjRegionDone!=1 &&rdObjEof!=1 );
  
  if((nReads+ret.l)>ret.m)
    realloc(ret,nReads+ret.l);


  //this is the awkward case this could cause an error in some very unlikely scenario.
  //whith the current buffer empty and the first read being a new chromosome. 
  //this is fixed good job monkey boy

  while(ret.l==0){
    bam1_t *b = ret.reads[0]=bam_init1();
    int tmp;

    if((tmp=bam_iter_read2(fp,it,b,hdr))<0){//FIXME
      if(tmp==-1){
	rdObjEof =1;
	//	ret.isEOF =1;
	isEof--;
      }else
	rdObjRegionDone =1;
      break;
    }
    ret.first[ret.l] = b->core.pos;
    ret.last[ret.l] = bam_endpos(b);
    ret.l++;
    nReads--;//breaking automaticly due to ret.l++
    
  }

  /*
    now do the general loop,
    1) read an alignemnt calulate start and end pos
    check
    a) if same chromosome that a new read has a geq FINE
    b) if same but < then UNSORTED
    
  */ 
  int i=0;
  int tmp;
  int buffed=0;
  while(1) {
    if(i+ret.l>=(ret.m-1)){
      ret.l += i;
      i=0;
      realloc(ret,ret.m+1);//will double buffer
    }
    
    bam1_t *b = ret.reads[i+ret.l]=bam_init1();
    if((tmp=bam_iter_read2(fp,it,b,hdr))<0) {
      if(tmp==-1){
	rdObjEof =1;
	isEof--;
      }else
	rdObjRegionDone =1;
      bam_destroy1(b);
      //DRAGON should we destroy b here?
      break;
    }
 
    //general fine case
    
    if((refToRead==b->core.tid)&&( b->core.pos >= ret.first[ret.l+i-1])&&(b->core.pos<stop)){
      ret.first[ret.l+i] = b->core.pos;
      ret.last[ret.l+i] = bam_endpos(b);
    }else if((refToRead==b->core.tid)&&b->core.pos>=stop){
      buffed=1;
      ret.bufferedRead = b;
      break;
    }
    else if(b->core.tid>refToRead){
      buffed=1;
      ret.bufferedRead = b;
      rdObjRegionDone =1;
      break;
    }else if(ret.first[ret.l+i-1]>b->core.pos){
      fprintf(stderr,"unsorted file detected will exit\n");
      fflush(stderr);
      exit(0);
    }
    i++;
  }
  ret.nReads =ret.l+i;
  ret.l = ret.nReads;
}


//nothing with buffered here
void read_reads_noStop(htsFile *fp,int nReads,int &isEof,sglPoolb &ret,int refToRead,iter_t *it,int &rdObjEof,int &rdObjRegionDone,bam_hdr_t *hdr) {
#if 0
  fprintf(stderr,"\t->[%s] buffRefid=%d\trefToRead=%d\n",__FUNCTION__,ret.bufferedRead.refID,refToRead);
#endif
  assert(rdObjEof==0 && ret.bufferedRead ==NULL);
 
  if((nReads+ret.l)>ret.m)
    realloc(ret,nReads+ret.l);
 
  //this is the awkward case this could cause an error in some very unlikely scenario.
  //whith the current buffer empty and the first read being a new chromosome. 
  //this is fixed good job monkey boy
  while(ret.l==0){
    bam1_t *b = ret.reads[0]=bam_init1();
    int tmp;
    if((tmp=bam_iter_read2(fp,it,b,hdr))<0){//FIXME
      if(tmp==-1){
	rdObjEof =1;
	isEof--;
      }else
	rdObjRegionDone =1;
      break;
    }
   
    //check that the read is == the refToread
    if(b->core.tid!=refToRead){
       ret.bufferedRead = b;
       rdObjRegionDone =1;
       return;
    }
    ret.first[ret.l] = b->core.pos;
    ret.last[ret.l] = bam_endpos(b);
    ret.l++;
    nReads--;
   
  }


  /*
    now do the general loop,
    1) read an alignemnt calulate start and end pos
    check
    a) if same chromosome that a new read has a geq FINE
    b) if same but < then UNSORTED
    
  */ 
  int i;
  int tmp;
  int buffed=0;
  for( i=0;i<nReads;i++){
    bam1_t *b = ret.reads[i+ret.l]=bam_init1();

    if((tmp=bam_iter_read2(fp,it,b,hdr))<0){
      if(tmp==-1){
	rdObjEof =1;
	isEof--;
      }else
	rdObjRegionDone =1;
      bam_destroy1(b);
      break;
    }

    //general fine case
    if((refToRead==b->core.tid)&&( b->core.pos >= ret.first[ret.l+i-1])){
      ret.first[ret.l+i] = b->core.pos;
      ret.last[ret.l+i] = bam_endpos(b);
    }else if(b->core.tid>refToRead){
      buffed=1;
      ret.bufferedRead = b;
      //      fprintf(stderr,"[%s] new chromosome detected will temporarliy stop reading at read # :%d\n",__FUNCTION__,i);
      rdObjRegionDone =1;
      break;
    }else if(ret.first[ret.l+i-1]>b->core.pos){
      fprintf(stderr,"[%s] unsorted file detected will exit new(-1)=%d new=%d,ret.l=%d i=%d\n",__FUNCTION__,ret.first[ret.l+i-1],b->core.pos,ret.l,i);
      exit(0);
    }
    
  }

  ret.nReads =ret.l+i;
  ret.l = ret.nReads;

}



//function will read data from all bamfiles, return value is the number of 'done' files
int collect_reads(bufReader *rd,int nFiles,int &notDone,sglPoolb *ret,int &readNlines,int ref,int &pickStop) {
#if 0
  fprintf(stderr,"\t[%s] Reading from referenceID=%d\n",__FUNCTION__,ref);
#endif

  int usedPicker=-1;
  for(int ii=0;ii<nFiles;ii++) {
    extern int *bamSortedIds;
    int i=bamSortedIds[ii];
    if(rd[i].regionDone||rd[i].isEOF){//if file is DONE with region go to next
      rd[i].regionDone = 1;
      continue;
    }
    
    int pre=ret[i].l;//number of elements before
    //function reads readNlines reads, from rd[i].fp and modifies isEOF and regionDone
    read_reads_noStop(rd[i].fp,readNlines,notDone,ret[i],ref,&rd[i].it,rd[i].isEOF,rd[i].regionDone,rd[i].hdr);
    
    //first check if reading caused and end of region event to occur
    if(rd[i].regionDone||rd[i].isEOF) 
      rd[i].regionDone = 1;
    
    if(ret[i].l>pre){//we could read data
      usedPicker = i;
      pickStop = ret[i].first[ret[i].l-1];
      break;
    }
  }
#if 0
  fprintf(stderr,"usedPicker:%d\n",usedPicker);
  exit(0);
#endif
 
  if(usedPicker==-1)  //<-this means we are done with the current chr/region
    return nFiles;

  //at this point we should have picked a picker. Thats a position=pickstop from a fileID=usedPicker that will be used as 'stopping point'
  for(int i=0;i<nFiles;i++) {
    if(rd[i].isEOF || rd[i].regionDone||i==usedPicker)
      continue;

#if 0
    fprintf(stderr,"i=%d regdone=%d\tiseof=%d\n",i,rd[i].regionDone,rd[i].isEOF);
    fprintf(stderr," getpool on i=%d\n",i);
#endif
    if(ret[i].l>0&&ret[i].first[ret[i].l-1]>pickStop)
      continue;
    read_reads_usingStopb(rd[i].fp,readNlines,notDone,ret[i],ref,&rd[i].it,pickStop,rd[i].isEOF,rd[i].regionDone,rd[i].hdr);
  } 
  int nDone =0;
  for(int i=0;i<nFiles;i++)
    if(rd[i].regionDone)
      nDone++;
  
  return nDone;

}
