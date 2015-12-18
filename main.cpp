//
//  main.cpp
//  Simulator
//
//  Created by Prakhar Malhotra on 9/27/15.
//  Copyright (c) 2015 Prakhar Malhotra. All rights reserved.
//
#include <iostream>
#include <fstream>
#include <string>
#include <bitset>
#include <math.h>
#include <iomanip>
#include <stdlib.h>

using namespace std;
using std::string;

void hexadecimaltodecimal(char* address);
void decimaltobinary(int);
void decimaltobinary2(int decimal);
void binarytodecimal(char *binary);
std::string conversion(char *);
std::string findmetagstring(std::string&, int);
std::string getactualtag(std::string &, int tagbits);
std::string findmesetstring(std::string& address,int setindexbits,int tagbits);
int findmesetnumber(std::string&, int, int);
int findmesetnumberreferenced(std::string&,int );
int charactertodecimal(char);
std::string maptohex(std::string & tempfour);

/*
 cacheelement is the fundamental unit of our cache.
 It has the block + valid bit + dirty bit + LRU counter
 */
class cacheelement{
public:
    int validbit, dirtybit, LRUcounter;
    std::string Tag,setreferencestring;
};

class cache{
public:
    int size, Associativity, Blocksize, VCEnabled, numberofsets;
    int blockbits,setindexbits,tagbits;
    int setreference;
    float reads, readmisses, writes, writemisses;
    int writehit, readhit;
    int match,nexttomemory;
    int key;
    int numberofvictimcacheblocks;
    float numberofswaps,swaprequests;
    int numberofwritebackstolowerlevel;
    char commandinputtype; //this is issued/set/passed by the upper level of the memory.Very important variable used by the miss handler.
    std::string tagreference,setreferencestring,passtolowerlevelread,passtolowerlevelevict;
    cacheelement ** matrix;
    cacheelement ** victimcache;
    void setthenumberofsetsinacache();
    void addressbitsfortagandsets();
    void referenceparametersfromaddress(std::string address);
    void cacheread(std::string & readaddress32 );
    void cachewrite(std::string & writeaddress32);
    void missroutinehandler(int );
    void make32bitstringtoread();
    void make32bitstringtoevict();
    void display();
    void eviction();
    /*variables to enable swap*/
    std::string victimfromcacheTag;
    int victimfromcachedirtybit;
    void swapcacheelements(int, int ,int );
    void swapVCelements(int , int );};
/*
 global variable
 */
cache * cacheaccess;
int memorytraffic;

void cache::make32bitstringtoread(){
    passtolowerlevelread = tagreference + setreferencestring;
    int k = passtolowerlevelread.length();
    for(int i=0;i<(32-k);i++)
        passtolowerlevelread=passtolowerlevelread + '0';
}

void cache::make32bitstringtoevict(){
    int k = passtolowerlevelevict.length();
    for(int i=0;i<(32-k);i++)
        passtolowerlevelevict=passtolowerlevelevict + '0';
}

void cache::cacheread(std::string & readaddress32){
    commandinputtype = 'r';
    reads ++;
    match = 0;
    int invalidbitsencounteredinatraversal = 0;
    int columnwherematchoccurs = 0;
    referenceparametersfromaddress(readaddress32);
    /*trying to match a tag in a set*/
    for(int columntraversalinaset = 0; columntraversalinaset<Associativity; columntraversalinaset++){
        if(matrix[setreference][columntraversalinaset].validbit == 1){
            if (matrix[setreference][columntraversalinaset].Tag == tagreference){
                match = 1;
                readhit++;
                columnwherematchoccurs = columntraversalinaset;
                break;
            }
        }
        else{
            invalidbitsencounteredinatraversal+=1;
        }
    }
    /*updating the LRU counter if a match happens*/
    if (match==1){
        for(int columntraversalinaset=0; columntraversalinaset<Associativity;columntraversalinaset++){
            if(matrix[setreference][columntraversalinaset].LRUcounter<matrix[setreference][columnwherematchoccurs].LRUcounter){
                matrix[setreference][columntraversalinaset].LRUcounter++;
            }
        }
        matrix[setreference][columnwherematchoccurs].LRUcounter=1;
    }
    /*
     routine handler called when a miss occurs.It is responsible for getting the block from the lower levels after evicting/not evicting a block in case of a conflict miss. It is also responsible for then fetching and placing the returned block at the appropriate place.
     */
    else{
        readmisses ++;
        if (invalidbitsencounteredinatraversal == 0){
            missroutinehandler(0);//ie eviction needs to take place and victim cache comes into the picture when enabled
        }
        else{
            missroutinehandler(1);
        }
    }
}

void cache::cachewrite(std::string & writeaddress32){
    commandinputtype = 'w';
    writes ++;
    match = 0;
    int invalidbitsencounteredinatraversal = 0;
    int columnwherematchoccurs = 0;
    referenceparametersfromaddress(writeaddress32);
    /*trying to match a tag in a set*/
    for(int columntraversalinaset = 0; columntraversalinaset<Associativity; columntraversalinaset++){
        if(matrix[setreference][columntraversalinaset].validbit == 1){
            if (matrix[setreference][columntraversalinaset].Tag == tagreference){
                match = 1;
                writehit++;
                matrix[setreference][columntraversalinaset].dirtybit=1;
                columnwherematchoccurs = columntraversalinaset;
                break;
            }
        }
        else{
            invalidbitsencounteredinatraversal+=1;
        }
    }
    /*updating the LRU counter if a match happens*/
    if (match==1){
        for(int columntraversalinaset=0; columntraversalinaset<Associativity;columntraversalinaset++){
            if(matrix[setreference][columntraversalinaset].LRUcounter<matrix[setreference][columnwherematchoccurs].LRUcounter){
                matrix[setreference][columntraversalinaset].LRUcounter++;
            }
        }
        matrix[setreference][columnwherematchoccurs].LRUcounter=1;
    }
    /*
     routine handler called when a miss occurs.It is responsible for getting the block from the lower levels after evicting/not evicting a block in case of a conflict miss. It is also responsible for then fetching and placing the returned block at the appropriate place.
     */
    else{
        writemisses ++;
        if (invalidbitsencounteredinatraversal == 0){
            missroutinehandler(0);//ie eviction needs to take place and the victim cache comes into the picture
        }
        else{
            missroutinehandler(1);
        }
    }
}
/*
 have to issue commands to the immediately lower level
 */
void cache::missroutinehandler(int simplemiss){
    int columnwhereinserted=0;
    /* no eviction*/
    if(simplemiss == 1){
        if(nexttomemory==1){
            memorytraffic++;
        }
        else{
            make32bitstringtoread();
            cacheaccess[key+1].cacheread(passtolowerlevelread);
        }
        for(int columntraversalinaset = 0; columntraversalinaset<Associativity; columntraversalinaset++){
            if(matrix[setreference][columntraversalinaset].validbit==0){
                matrix[setreference][columntraversalinaset].validbit=1;
                matrix[setreference][columntraversalinaset].Tag = tagreference;
                columnwhereinserted = columntraversalinaset;
                if(commandinputtype=='w'){
                    matrix[setreference][columntraversalinaset].dirtybit=1;
                }
                else{
                    matrix[setreference][columntraversalinaset].dirtybit=0;
                }
                break;
            }
        }
        for(int columntraversalinaset = 0; columntraversalinaset<Associativity;columntraversalinaset++){
            if(columntraversalinaset==columnwhereinserted){
                matrix[setreference][columntraversalinaset].LRUcounter=1;
            }
            else{
                matrix[setreference][columntraversalinaset].LRUcounter++;//adding counter for invalid bits doesn't matter
            }
        }
        
    }
    /*eviction*/
    else{
        if(VCEnabled==1){
            swaprequests++;
            int numberofinvalidbitsencounteredinVC = 0,columnwhereblockevictedfrom=0,victimcacheblockwhereswapped,victimcacheblockwhereinserted,victimcacheblockevicted=0;
            for(int columntraversalinaset=0; columntraversalinaset<Associativity;columntraversalinaset++){
                if((matrix[setreference][columntraversalinaset].LRUcounter == Associativity)&&(match==0)){
                    columnwhereblockevictedfrom = columntraversalinaset;
                }
            }
            
            //check to see if swap possible
            for(int victimcachetraversal=0; victimcachetraversal<numberofvictimcacheblocks;victimcachetraversal++){
                if(victimcache[0][victimcachetraversal].validbit==1){
                    if((tagreference==victimcache[0][victimcachetraversal].Tag)&&(setreferencestring == victimcache[0][victimcachetraversal].setreferencestring)){
                        victimfromcacheTag = matrix[setreference][columnwhereblockevictedfrom].Tag;
                        victimfromcachedirtybit = matrix[setreference][columnwhereblockevictedfrom].dirtybit;
                        matrix[setreference][columnwhereblockevictedfrom].Tag = victimcache[0][victimcachetraversal].Tag;
                        matrix[setreference][columnwhereblockevictedfrom].dirtybit = victimcache[0][victimcachetraversal].dirtybit;
                        victimcache[0][victimcachetraversal].Tag = victimfromcacheTag;
                        victimcache[0][victimcachetraversal].dirtybit = victimfromcachedirtybit;
                        victimcacheblockwhereswapped = victimcachetraversal;
                        //swap complete
                        numberofswaps++;
                        if(commandinputtype=='w'){
                            matrix[setreference][columnwhereblockevictedfrom].dirtybit=1;
                        }
                        //update LRU for the victim cache
                        for(int k=0; k<numberofvictimcacheblocks;k++){
                            if(victimcache[0][k].validbit==1){
                                if(victimcache[0][k].LRUcounter<victimcache[0][victimcacheblockwhereswapped].LRUcounter){
                                    victimcache[0][k].LRUcounter++;
                                }
                            }
                        }
                        victimcache[0][victimcacheblockwhereswapped].LRUcounter = 1;
                        match = 1;
                        break;
                    }
                }
                else{
                    numberofinvalidbitsencounteredinVC++;
                }
            }
            
            if((numberofinvalidbitsencounteredinVC==0)&&(match==0)){
                //complex VC miss routine
                //find the LRU VC block to be evicted
                for(int i = 0;i<numberofvictimcacheblocks;i++){
                    if(victimcache[0][i].LRUcounter==numberofvictimcacheblocks){
                        victimcacheblockevicted=i;
                    }
                }
                //eviction from VC
                if(victimcache[0][victimcacheblockevicted].dirtybit==1){
                   numberofwritebackstolowerlevel ++;
                    if(nexttomemory==1){
                        memorytraffic++;
                        victimcache[0][victimcacheblockevicted].validbit = 0;
                    }
                    else{
                        passtolowerlevelevict = victimcache[0][victimcacheblockevicted].Tag + victimcache[0][victimcacheblockevicted].setreferencestring;
                        make32bitstringtoevict();
                        cacheaccess[key+1].cachewrite(passtolowerlevelevict);
                        victimcache[0][victimcacheblockevicted].dirtybit=0;
                        victimcache[0][victimcacheblockevicted].validbit=0;
                    }
                }
                //evict from main cache to victim cache
                victimcache[0][victimcacheblockevicted].validbit=1;
                victimcache[0][victimcacheblockevicted].Tag = matrix[setreference][columnwhereblockevictedfrom].Tag ;
                victimcache[0][victimcacheblockevicted].setreferencestring=setreferencestring;
                victimcache[0][victimcacheblockevicted].dirtybit = matrix[setreference][columnwhereblockevictedfrom].dirtybit;
                
                //fetch block from lower level
                if(nexttomemory==1){
                    memorytraffic++;
                    matrix[setreference][columnwhereblockevictedfrom].validbit=1;
                    matrix[setreference][columnwhereblockevictedfrom].Tag=tagreference;
                    if(commandinputtype=='w'){
                        matrix[setreference][columnwhereblockevictedfrom].dirtybit=1;
                    }
                    else{
                        matrix[setreference][columnwhereblockevictedfrom].dirtybit=0;
                    }
                }
                else{
                    make32bitstringtoread();
                    cacheaccess[key+1].cacheread(passtolowerlevelread);
                    matrix[setreference][columnwhereblockevictedfrom].Tag = tagreference;
                    if(commandinputtype=='w'){
                        matrix[setreference][columnwhereblockevictedfrom].dirtybit=1;
                    }
                    else{
                        matrix[setreference][columnwhereblockevictedfrom].dirtybit=0;
                    }
                }
                /*update LRU for victimcache*/
                for(int k=0; k<numberofvictimcacheblocks;k++){
                    if(k==victimcacheblockevicted){
                        victimcache[0][k].LRUcounter=1;
                    }
                    else{
                        victimcache[0][k].LRUcounter++;
                    }
                }
                
                match=1;
            }
            
            else if (match==0){
                //simple VC miss routine ie VC has space for main cache victim block
                for(int victimcachetraversal=0; victimcachetraversal<numberofvictimcacheblocks;victimcachetraversal++){
                    //eviction from main cache to VC complete of LRU
                    if(victimcache[0][victimcachetraversal].validbit==0){
                        victimcache[0][victimcachetraversal].validbit=1;
                        victimcache[0][victimcachetraversal].Tag=matrix[setreference][columnwhereblockevictedfrom].Tag;
                        victimcache[0][victimcachetraversal].setreferencestring = setreferencestring;
                        victimcache[0][victimcachetraversal].dirtybit=matrix[setreference][columnwhereblockevictedfrom].dirtybit;
                        victimcacheblockwhereinserted = victimcachetraversal;
                        
                        //fetch the block from lower level
                        if(nexttomemory==1){
                            memorytraffic++; 
                            matrix[setreference][columnwhereblockevictedfrom].validbit=1;
                            matrix[setreference][columnwhereblockevictedfrom].Tag=tagreference;
                            if(commandinputtype=='w'){
                                matrix[setreference][columnwhereblockevictedfrom].dirtybit=1;
                            }
                            else{
                                matrix[setreference][columnwhereblockevictedfrom].dirtybit=0;
                            }
                        }
                        else{
                            make32bitstringtoread();
                            cacheaccess[key+1].cacheread(passtolowerlevelread);
                            matrix[setreference][columnwhereblockevictedfrom].Tag = tagreference;
                            if(commandinputtype=='w'){
                                matrix[setreference][columnwhereblockevictedfrom].dirtybit=1;
                            }
                            else{
                                matrix[setreference][columnwhereblockevictedfrom].dirtybit=0;
                            }
                        }
                        //update LRU for the victim cache
                        for(int k=0; k<numberofvictimcacheblocks;k++){
                            if(k==victimcacheblockwhereinserted){
                                victimcache[0][k].LRUcounter=1;
                            }
                            else if (victimcache[0][k].validbit==1){
                                victimcache[0][k].LRUcounter++;
                            }
                        }
                        match=1;
                        break;
                    }
                }
            }
            //update LRU for the main cache
            for(int k=0; k<Associativity;k++){
                if(k==columnwhereblockevictedfrom){
                    matrix[setreference][k].LRUcounter=1;
                }
                else{
                    matrix[setreference][k].LRUcounter++;
                }
            }
        }
        else{
            for(int columntraversalinaset=0; columntraversalinaset<Associativity;columntraversalinaset++){
                if(matrix[setreference][columntraversalinaset].LRUcounter == Associativity){
                    if(nexttomemory==0){
                        if(matrix[setreference][columntraversalinaset].dirtybit==1){
                            passtolowerlevelevict = matrix[setreference][columntraversalinaset].Tag + setreferencestring;
                            make32bitstringtoevict();
                            numberofwritebackstolowerlevel ++;
                            cacheaccess[key+1].cachewrite(passtolowerlevelevict);
                            matrix[setreference][columntraversalinaset].dirtybit=0;
                        }
                        make32bitstringtoread();
                        cacheaccess[key+1].cacheread(passtolowerlevelread);
                        matrix[setreference][columntraversalinaset].Tag = tagreference;
                        columnwhereinserted = columntraversalinaset;
                        match = 1;
                        if(commandinputtype=='w'){
                            matrix[setreference][columntraversalinaset].dirtybit=1;
                        }
                        else{
                            matrix[setreference][columntraversalinaset].dirtybit=0;
                        }
                        break;
                    }
                    else{
                        memorytraffic++;
                        if(matrix[setreference][columntraversalinaset].dirtybit==1){
                           numberofwritebackstolowerlevel ++;
                           memorytraffic++;
                        }
                        matrix[setreference][columntraversalinaset].dirtybit=0;
                        matrix[setreference][columntraversalinaset].Tag = tagreference;
                        columnwhereinserted = columntraversalinaset;
                        match = 1;
                        if(commandinputtype=='w'){
                            matrix[setreference][columntraversalinaset].dirtybit=1;
                        }
                        else{
                            matrix[setreference][columntraversalinaset].dirtybit=0;
                        }
                        break;
                    }
                }
            }
            for(int columntraversalinaset = 0; columntraversalinaset<Associativity;columntraversalinaset++){
                if(columntraversalinaset==columnwhereinserted){
                    matrix[setreference][columntraversalinaset].LRUcounter=1;
                }
                else{
                    matrix[setreference][columntraversalinaset].LRUcounter++;
                }
            }
        }
    }
}

void configurecacheselements(cache &);
cache * initializecaches(int ,int ,int ,int, int, int);
std::string trace_file;

int main(int argc, const char * argv[]) {
    int commonblocksize, L1size,L1associativity, VCnumberofblocks, L2size, L2associativity;
    
    if(argc==8){
        commonblocksize = atoi(argv[1]);
        L1size = atoi(argv[2]);
        L1associativity = atoi(argv[3]);
        VCnumberofblocks = atoi(argv[4]);
        L2size = atoi(argv[5]);
        L2associativity = atoi(argv[6]);
        trace_file = argv[7];
    }
    //std::cout << commonblocksize << L1size << L1associativity << VCnumberofblocks << L2size << L2associativity;
    /*
     reference to the caches created
     */
    cache* cacheaddress;
    cacheaddress = initializecaches(commonblocksize, L1size, L1associativity, VCnumberofblocks, L2size, L2associativity);
    //std::string testaddress = "00000011110010100101010101010111";
    /*
     open file and start passing commands to the caches in a hierarchical fashion
     */
    std::string line,addressbinary;
    char address[9];
    std::ifstream tracefile;
    tracefile.open(trace_file.c_str());
    if(tracefile.is_open()){
        while (getline (tracefile,line)){
            
            line.insert(2,(10-line.length()),'0');
            //std::cout << line <<'\n';
            for(int i=2; i<=line.length()-1; i++){
                address[i-2] = line[i];
            }
            addressbinary = conversion(address);
            if(line[0]=='r'){
                cacheaddress[0].cacheread(addressbinary);
            }
            else{
                cacheaddress[0].cachewrite(addressbinary);
            }
        }
    }
    
    std::cout<<"===== Simulator configuration =====" << "\n";
    std::cout<<" BLOCKSIZE:			        "<< commonblocksize << "\n";
    std::cout<<" L1_SIZE:			        "<< L1size << "\n";
    std::cout<<" L1_ASSOC:			        "<< L1associativity<<"\n";
    std::cout<<" VC_NUM_BLOCKS:  		        "<< VCnumberofblocks<<"\n";
    std::cout<<" L2_SIZE:			        "<< L2size << "\n";
    std::cout<<" L2_ASSOC:			        "<< L2associativity<<"\n";
    std::cout<<" trace_file:      		        "<<trace_file<<"\n";
    
    std::cout<<"\n";
    std::cout<<"===== L1 contents ====="<<"\n";cacheaddress[0].display();
    if(L2size>0){
    std::cout<<"\n";
    std::cout<<"===== L2 contents ====="<<"\n";cacheaddress[1].display();
    }
    std::cout<<"\n";
    std::cout<<"===== Simulation results ====="<<"\n";
    std::cout<<"  a. numberof L1 reads:			"<< cacheaddress[0].reads <<"\n";
    std::cout<<"  b. number of L1 read misses:		"<< cacheaddress[0].readmisses <<"\n";
    std::cout<<"  c. number of L1 writes:		"<< cacheaddress[0].writes <<"\n";
    std::cout<<"  d. number of L1 write misses:		"<< cacheaddress[0].writemisses <<"\n";
    std::cout<<"  e. number of swap requests:		"<< cacheaddress[0].swaprequests <<"\n";
    cout.precision(4);
    cout.setf(ios::fixed, ios::floatfield);
    std::cout<<"  f. swap request rate:	 		"<< cacheaddress[0].swaprequests/(cacheaddress[0].reads + cacheaddress[0].writes)<<"\n";
    cout.precision(0);
    cout.setf(ios::fixed, ios::floatfield);
    std::cout<<"  g. number of swaps:	 		"<< cacheaddress[0].numberofswaps<<"\n";
    cout.precision(4);
    cout.setf(ios::fixed, ios::floatfield);
    std::cout<<"  h. combined L1+VC miss rate:		"<<(cacheaddress[0].readmisses + cacheaddress[0].writemisses - cacheaddress[0].numberofswaps)/(cacheaddress[0].reads + cacheaddress[0].writes)<< "\n";
    cout.precision(0);
    cout.setf(ios::fixed, ios::floatfield);
    std::cout<<"  i. number writebacks from L1/VC:	"<< cacheaddress[0].numberofwritebackstolowerlevel<<"\n";
    std::cout<<"  j. number of L2 reads:	 	"<< cacheaddress[1].reads <<"\n";
    std::cout<<"  k. number of L2 read misses:		"; if(L2size>0)std::cout << cacheaddress[1].readmisses << "\n";else std::cout << " 0 "<<"\n"; 
    std::cout<<"  l. number of L2 writes:		"<< cacheaddress[1].writes <<"\n";
    std::cout<<"  m. number of L2 write misses:		"; if(L2size>0)std::cout << cacheaddress[1].writemisses << "\n";else std::cout << " 0 "<<"\n";
    cout.precision(4);
    cout.setf(ios::fixed, ios::floatfield);
    std::cout<<"  n. L2 miss rate:			";if(L2size>0)std::cout << (cacheaddress[1].readmisses/cacheaddress[1].reads) << "\n";else std::cout << " 0.0000 " << "\n";
    std::cout<<"  o. number of writebacks from L2:      "<< cacheaddress[1].numberofwritebackstolowerlevel << "\n";
    std::cout<<"  p. total memory traffic:	        " << memorytraffic << "\n";
    return 0;
}

/*
 function to take input parameters from the user about the number of caches, their size, associativity
 and blocksize
 */

cache* initializecaches(int commonblocksize, int L1size, int L1associativity, int VCnumberofblocks, int L2size, int L2associativity){
    int a;
    if(L2size == 0){
        a = 1;
    }
    else{
        a = 2;
    }
    cacheaccess = new cache[a];//dynamically allocate the number of caches and cacheaccess points to the first element of the cache array
    cacheaccess[0].size = L1size;
    cacheaccess[0].Associativity = L1associativity;
    cacheaccess[0].Blocksize = commonblocksize;
    if(VCnumberofblocks==0){
        cacheaccess[0].VCEnabled = 0;
    }
    else{
        cacheaccess[0].VCEnabled = 1;
        cacheaccess[0].numberofvictimcacheblocks = VCnumberofblocks;
    }
    cacheaccess[0].setthenumberofsetsinacache();
    cacheaccess[0].addressbitsfortagandsets();
    configurecacheselements(cacheaccess[0]);
    if(0==(a-1))
        cacheaccess[0].nexttomemory = 1;
    cacheaccess[0].key = 0;
    cacheaccess[0].reads = 0;
    cacheaccess[0].writes = 0;
    
    if(a>1){
        /*cacheL2*/
        cacheaccess[1].size = L2size;
        cacheaccess[1].Associativity = L2associativity;
        cacheaccess[1].Blocksize = commonblocksize;
        cacheaccess[1].VCEnabled = 0;
        cacheaccess[1].setthenumberofsetsinacache();
        cacheaccess[1].addressbitsfortagandsets();
        configurecacheselements(cacheaccess[1]);
        if(1==(a-1))
            cacheaccess[1].nexttomemory = 1;
        cacheaccess[1].key = 1;
        cacheaccess[1].reads = 0;
        cacheaccess[1].writes = 0;
    }
    
    return cacheaccess;
}

/*
 function to dynamically allocate memory for the cachelements in a cache
 */

void configurecacheselements(cache &cacheaccess)
{
    int i;
    cacheaccess.matrix = new cacheelement*[cacheaccess.numberofsets];
    for(i = 0; i<(cacheaccess.numberofsets);i++)
    {
        cacheaccess.matrix[i]= new cacheelement[cacheaccess.Associativity];
    }
    if(cacheaccess.VCEnabled==1){
        cacheaccess.victimcache = new cacheelement*[1];//this is because a victim cache is fully associative
        for(i=0;i<(1);i++){
            cacheaccess.victimcache[i] = new cacheelement[cacheaccess.numberofvictimcacheblocks];
        }
    }
}

/*
 the following returns the 32 bit address string of the 8 character long address string
 */
std::string conversion(char * address){
    int k[9];
    std::string combinedstring = "";
    /*
     conversion steps once we have an address
     */
    for(int l=0;l<=7;l++){
        if ((address[l]=='a')|(address[l]=='b')|(address[l]=='c')|(address[l]=='d')|(address[l]=='e')|(address[l]=='f'))
            k[l] = address[l]-87;
        else
            k[l] = address[l]-48;
        std::string binary = std::bitset<4>(k[l]).to_string();
        combinedstring = combinedstring + binary;
    }
    return std::string(combinedstring);
}

std::string findmetagstring(std::string& address,int tagbits){
    std::string Tag = "";
    std::string ActualTag;
    //std::cout<< address<< "\n";
    for(int p=0;p<=(tagbits-1);p++){
        Tag = Tag + address[p];
    }
    return Tag;
}
std::string findmesetstring(std::string& address,int setindexbits,int tagbits){
    std::string set = "";
    for(int p=tagbits;p<=(tagbits+setindexbits-1);p++){
        set = set + address[p];
    }
    return(set);
}

int findmesetnumber(std::string& address,int setindexbits,int tagbits){
    std::string set = "";
    int setreferenced;
    //std::cout<< address<< "\n";
    for(int p=tagbits;p<=(tagbits+setindexbits-1);p++){
        set = set + address[p];
    }
    setreferenced = findmesetnumberreferenced(set, setindexbits);
    return (setreferenced);
}

int findmesetnumberreferenced(std::string& stringset,int setindexbits){
    int k,setreference=0;
    for(k=setindexbits-1;k>=0;k--){
        setreference = setreference + charactertodecimal(stringset[k])*pow(2,(setindexbits-k-1));
    }
    return setreference;
}

int charactertodecimal(char character){
    int k;
    if ((character=='a')|(character=='b')|(character=='c')|(character=='d')|(character=='e')|(character=='f'))
        k = character-87;
    else
        k = character-48;
    return k;
}

/****************************************cacheclassfunctionslocked**************************************/
/*
 one time use class function to set the number of sets in a cache based on the values for other parameters from the user
 */
void cache::setthenumberofsetsinacache(){
if((Associativity * Blocksize)>0)
    numberofsets = size/(Associativity * Blocksize);
    //std::cout << size << "SIZE \n";
    //std::cout << Associativity << "Associativity";
    //std::cout << Blocksize << "Blocksize \n";
    //std::cout << numberofsets << "SETS \n"; 
}
/*
 class function to set the referenced parameters by the instruction issued to the cache either from the processor or from the upper level memory using the address 8 character string long
 */
void cache::referenceparametersfromaddress(std::string combined32bitstring){
    tagreference = findmetagstring(combined32bitstring,tagbits);
    setreference = findmesetnumber(combined32bitstring,setindexbits,tagbits);
    setreferencestring = findmesetstring(combined32bitstring,setindexbits,tagbits);
}

void cache::addressbitsfortagandsets(){
    blockbits = log2(Blocksize);
    setindexbits = log2(numberofsets);
    tagbits = 32 - blockbits - setindexbits;
}

//write the sequence for formatting later
void cache::display(){
    int k,j;
    int decimal = 0;
    int p;
    int i=0;
    std::string totalstring="";
    for(int set =0; set<numberofsets;set++){
        for(int i=0;i<Associativity;i++){
            for(j = i+1; j<Associativity;j++){
                if(matrix[set][i].LRUcounter>matrix[set][j].LRUcounter){
                    swapcacheelements(set, i, j);
                }
            }
        }
    }
    for(int set=0;set<numberofsets;set++){
          std::cout <<std::setbase(10);
     std::cout << "  set"<<" "<< set << ":"<<" ";
        for(int columntraversal =0; columntraversal<Associativity;columntraversal++){
            if (matrix[set][columntraversal].validbit == 1) {
                for(k=tagbits-1;k>=0;k--){
                p = (matrix[set][columntraversal].Tag[k])-48;
                decimal = decimal + p*pow(2,-k+tagbits-1);
            }  
            std::cout << std::setbase(16);
            std::cout << " " << decimal << " ";
           // std::cout << matrix[set][columntraversal].LRUcounter<<"\n ";
            if(matrix[set][columntraversal].dirtybit== 1){
                std::cout  <<"D "<<" " ;
            }
            else
            {
            	  std::cout  <<"  "<<" " ;
            }
            decimal = 0;
           }
           else{
               std::cout << " " ;
               }
        }
        std::cout<<"\n";
    }
    
    std::cout <<std::setbase(10);

    if(VCEnabled==1){
        std::cout <<"\n";
        std::cout<<"===== VC contents ====="<<"\n";
        {
                for(int i=0;i<numberofvictimcacheblocks;i++){
                    for(j = i+1; j<numberofvictimcacheblocks;j++){
                        if(victimcache[0][i].LRUcounter>victimcache[0][j].LRUcounter){
                            swapVCelements(i,j);
                        }
                    }
                }
        std::cout << "  " << "set 0:";
        for(int columntraversal = 0; columntraversal<numberofvictimcacheblocks;columntraversal++){
            i=0;
            totalstring=victimcache[0][columntraversal].Tag+victimcache[0][columntraversal].setreferencestring;
            for(k=tagbits+setindexbits-1;k>=0;k--){
                p = (totalstring[k])-48;
                decimal = decimal + p*pow(2,i);
                i++;
           }
            //std::cout << "set"<<"   "<< 0 << ":"<<"   ";
            std::cout << std::setbase(16);
            std::cout << " " << decimal << "   ";
            std::cout <<std::setbase(10);
            //std::cout << victimcache[0][columntraversal].LRUcounter << "  ";
            if(victimcache[0][columntraversal].dirtybit== 1){
                std::cout  <<"D"<<" " ;
            }
            decimal = 0;
            totalstring="";
        }
      }
      std::cout<<"\n";
    }
    
    std::cout<<std::setbase(10);
}

void cache::swapcacheelements(int set, int index1, int index2){
    std::string holdingtag;
    int holdingLRUcounter;
    int holdingvalidbit;
    int holdingdirtybit;
    
    holdingtag = matrix[set][index1].Tag;
    holdingLRUcounter = matrix[set][index1].LRUcounter;
    holdingvalidbit = matrix[set][index1].validbit;
    holdingdirtybit = matrix[set][index1].dirtybit;
    
    matrix[set][index1].Tag = matrix[set][index2].Tag;
    matrix[set][index1].LRUcounter = matrix[set][index2].LRUcounter;
    matrix[set][index1].validbit = matrix[set][index2].validbit;
    matrix[set][index1].dirtybit = matrix[set][index2].dirtybit;
    
    matrix[set][index2].Tag = holdingtag;
    matrix[set][index2].LRUcounter = holdingLRUcounter;
    matrix[set][index2].validbit = holdingvalidbit;
    matrix[set][index2].dirtybit = holdingdirtybit;
}

void cache::swapVCelements(int index1, int index2){
    std::string holdingtag,holdingsetreferencestring;
    int holdingLRUcounter;
    int holdingvalidbit;
    int holdingdirtybit;
    
    holdingtag = victimcache[0][index1].Tag;
    holdingLRUcounter = victimcache[0][index1].LRUcounter;
    holdingvalidbit = victimcache[0][index1].validbit;
    holdingdirtybit = victimcache[0][index1].dirtybit;
    holdingsetreferencestring = victimcache[0][index1].setreferencestring;
    
    victimcache[0][index1].Tag = victimcache[0][index2].Tag;
    victimcache[0][index1].LRUcounter = victimcache[0][index2].LRUcounter;
    victimcache[0][index1].validbit = victimcache[0][index2].validbit;
    victimcache[0][index1].dirtybit = victimcache[0][index2].dirtybit;
    victimcache[0][index1].setreferencestring = victimcache[0][index2].setreferencestring;
    
    victimcache[0][index2].Tag = holdingtag;
    victimcache[0][index2].LRUcounter = holdingLRUcounter;
    victimcache[0][index2].validbit = holdingvalidbit;
    victimcache[0][index2].dirtybit = holdingdirtybit;
    victimcache[0][index2].setreferencestring = holdingsetreferencestring;
}




