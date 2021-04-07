/*
Copyright (C) 2019-2020 JingWeiZhangHuai <jingweizhanghuai@163.com>
Licensed under the Apache License, Version 2.0; you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "morn_util.h"

struct HandleSheetCreate
{
    MSheet *sheet;
    MChain *property;
    int row;
    
    int *col;
    void ***data;
    int *num;
    MInfo *row_info;
    
    MMemory *memory;

    // MList *list;
};
void endSheetCreate(void *info)
{
    struct HandleSheetCreate *handle = (struct HandleSheetCreate *)info;
    mException((handle->sheet == NULL),EXIT,"invalid sheet");
    if(handle->property!=NULL) mChainRelease(handle->property);
    if(handle->data!=NULL)
    {
        for(int j=0;j<handle->row;j++)
        {
            if(handle->data[j] != NULL)
                mFree(handle->data[j]);
        }
        mFree(handle->data);
    }
    
    if(handle->col != NULL) mFree(handle->col);
    if(handle->num != NULL) mFree(handle->num);
    if(handle->row_info != NULL) mFree(handle->row_info);
    if(handle->memory !=NULL) mMemoryRelease(handle->memory);
    
    mFree(handle->sheet);
}
#define HASH_SheetCreate 0x3cb067ca
MSheet *SheetCreate(int row,int *col,void ***data)
{
    MSheet *sheet = (MSheet *)mMalloc(sizeof(MSheet));
    memset(sheet,0,sizeof(MSheet));

    sheet->handle = mHandleCreate();
    MHandle *hdl=mHandle(sheet,SheetCreate);
    struct HandleSheetCreate *handle = (struct HandleSheetCreate *)(hdl->handle);
    handle->sheet = sheet;
    
    if(row<0) row = 0;
    handle->row = row;
    sheet->row = row;
    
    if(row == 0) 
    {
        mException((!INVALID_POINTER(col))||(!INVALID_POINTER(data)),EXIT,"invalid input");
        sheet->col = NULL;
        sheet->data = NULL;
        sheet->row_info = NULL;
        return sheet;
    }
    
    handle->data    = (void ***)mMalloc(row*sizeof(void **));
    handle->num     = (int *   )mMalloc(row*sizeof(int));
    handle->col     = (int *   )mMalloc(row*sizeof(int));
    handle->row_info= (MInfo * )mMalloc(row*sizeof(MInfo));
    memset(handle->row_info,0,row*sizeof(MInfo));
    if(!INVALID_POINTER(col))
    {
        for(int j=0;j<row;j++)
        {
            if(col[j] <= 0) 
            {
                handle->data[j]= NULL;
                handle->num[j] = 0;
                handle->col[j] = 0;
                continue;
            }
            handle->data[j] = (void **)mMalloc(col[j]*sizeof(void *));
            handle->col[j] = col[j];
            handle->num[j] = col[j];
            
            if(!INVALID_POINTER(data))
                memcpy(handle->data[j],data[j],col[j]*sizeof(void *));
            else
                memset(handle->data[j],0      ,col[j]*sizeof(void *));
        }
    }
    else
    {
        mException((!INVALID_POINTER(data)),EXIT,"invalid input");
        memset(handle->data,0,row*sizeof(void **));
        memset(handle->num ,0,row*sizeof(int));
        memset(handle->col ,0,row*sizeof(int));
    }

    sheet->col = handle->col;
    sheet->data= handle->data;
    sheet->row_info = handle->row_info;
    
    return sheet;
}

void mSheetRelease(MSheet *sheet)
{
    mException(INVALID_POINTER(sheet),EXIT,"invalid input source sheet");
    if(!INVALID_POINTER(sheet->handle))
        mHandleRelease(sheet->handle);
}

void mSheetRowAppend(MSheet *sheet,int n)
{
    mException(INVALID_POINTER(sheet),EXIT,"invalid input source sheet");
    if(n<0) n = sheet->row + 1;
   
    struct HandleSheetCreate *handle= (struct HandleSheetCreate *)(((MHandle *)(sheet->handle->data[0]))->handle);
    if(n>handle->row)
    {
        int row = sheet->row + MAX(n-sheet->row,8);
        void ***handle_data   =(void ***)mMalloc(row*sizeof(void **));
        int    *handle_num    =   (int *)mMalloc(row*sizeof(int));
        int    *handle_col    =   (int *)mMalloc(row*sizeof(int));
        MInfo *handle_row_info= (MInfo *)mMalloc(row*sizeof(MInfo));
        if(handle->row >0)
        {
            memcpy(handle_data    ,handle->data    ,handle->row*sizeof(void **));
            memcpy(handle_num     ,handle->num     ,handle->row*sizeof(int));
            memcpy(handle_col     ,handle->col     ,handle->row*sizeof(int));
            memcpy(handle_row_info,handle->row_info,handle->row*sizeof(MInfo));
        }
        memset(handle_data    +handle->row,0,(row-handle->row)*sizeof(void **));
        memset(handle_num     +handle->row,0,(row-handle->row)*sizeof(int));
        memset(handle_col     +handle->row,0,(row-handle->row)*sizeof(int));
        memset(handle_row_info+handle->row,0,(row-handle->row)*sizeof(MInfo));
        
        if(handle->data    !=NULL) {mFree(handle->data);    } handle->data    =handle_data;
        if(handle->num     !=NULL) {mFree(handle->num);     } handle->num     =handle_num;
        if(handle->col     !=NULL) {mFree(handle->col);     } handle->col     =handle_col;
        if(handle->row_info!=NULL) {mFree(handle->row_info);} handle->row_info=handle_row_info;
        
        sheet->data    = handle->data;
        sheet->col     = handle->col;
        sheet->row_info= handle->row_info;
        
        handle->row = row;
    }
    sheet->row = n;
}

void mSheetColAppend(MSheet *sheet,int row,int n)
{
    mException(INVALID_POINTER(sheet),EXIT,"invalid input source sheet");
    mException((row<0)||(row>=sheet->row),EXIT,"invalid input row");
    if(n<0) n = sheet->col[row]+1;
    
    struct HandleSheetCreate *handle= (struct HandleSheetCreate *)(((MHandle *)(sheet->handle->data[0]))->handle);
    if(n > handle->num[row])
    {
        int col = sheet->col[row] + MAX(n-sheet->col[row],8);
        
        void **sheet_data = (void **)mMalloc(col*sizeof(void *));
        if(sheet->col[row] >0)
            memcpy(sheet_data,sheet->data[row],sheet->col[row]*sizeof(void *));
        memset(sheet_data + sheet->col[row],0,(col - sheet->col[row])*sizeof(void *));
        
        if(sheet->data[row]!=NULL) mFree(sheet->data[row]);
   
        handle->data[row] = sheet_data;
        handle->num[row] = col;
        sheet->data[row] = handle->data[row];
    }
    
    sheet->col[row] = n;
}

void mSheetPlace(MSheet *sheet,void *data,int row,int col,int size)
{
    if((row<=0)||(col<=0)) return;
    mException((size<=0),EXIT,"invalid input list element size");

    mSheetRowAppend(sheet,row);
    for(int j=0;j<row;j++) mSheetColAppend(sheet,j,col);

    void **idx = (void **)mMalloc(row*col*sizeof(void *));
    struct HandleSheetCreate *handle = (struct HandleSheetCreate *)(((MHandle *)(sheet->handle->data[0]))->handle);

    if(handle->memory == NULL) handle->memory = mMemoryCreate(1,row*col*size,MORN_HOST);
    else mMemoryAppend(handle->memory,row*col*size);
    mMemoryIndex(handle->memory,row*col,size,&idx,1);

    char *p_data=(char *)data;void **p_idx = idx;
    for(int j=0;j<row;j++)
    {
        memcpy(sheet->data[j],p_idx,col*sizeof(void *));p_idx=p_idx+col;
        if(data!=NULL)
        {
            for(int i=0;i<col;i++){memcpy(sheet->data[j][i],p_data,size);p_data+=size;}
        } 
    }
    mFree(idx);
}

void mSheetOperate(MSheet *sheet,void (*func)(void *,void *),void *para)
{
    mException((sheet==NULL)||(func==NULL),EXIT,"invalid input");
    for(int j=0;j<sheet->row;j++)for(int i=0;i<sheet->col[j];i++)
        func(sheet->data[j][i],para);
}

void mSheetClear(MSheet *sheet)
{
    for(int r=0;r<sheet->row;r++) sheet->col[r]=0;
    sheet->row=0;
    struct HandleSheetCreate *handle0 = (struct HandleSheetCreate *)(((MHandle *)(sheet->handle->data[0]))->handle);
    if(handle0->memory!=NULL) mMemoryClear(handle0->memory);
}

struct HandleSheetRowList
{
    MList **list;
    int list_num;
    int order;
};
void endSheetRowList(struct HandleSheetRowList *handle)
{
    if(handle->list_num>0)
    {
        for(int i=0;i<handle->list_num;i++)
        {
            if(handle->list[i]!=NULL) mListRelease(handle->list[i]);
        }
        mFree(handle->list);
    }
}
#define HASH_SheetRowList 0x40b550d6
MList *mSheetRowList(MSheet *sheet,int row)
{
    mException(INVALID_POINTER(sheet),EXIT,"invalid sheet");
    mException(row>=sheet->row,EXIT,"invalid input");
    MHandle *hdl = mHandle(sheet,SheetRowList);
    struct HandleSheetRowList *handle = (struct HandleSheetRowList *)(hdl->handle);
    if(row<0)
    {
        row=handle->order;
        if(row==sheet->row) return NULL;
    }
    if((hdl->valid == 0)||(row>=handle->list_num))
    {
        MList **list = mMalloc(sheet->row*sizeof(MList *));memset(list,0,sheet->row*sizeof(MList *));
        if(handle->list_num>0) memcpy(list,handle->list,handle->list_num*sizeof(MList *));
        handle->list = list;
        handle->list_num = sheet->row;
        hdl->valid = 1;
    }
    handle->order = row+1;

    mListClear(handle->list[row]);
    mListAppend(handle->list[row],sheet->data[row],sheet->col[row]);
    return handle->list[row];
}

struct HandleSheetWrite
{
    int write_size;
};
void endSheetWrite(void *info) {}
#define HASH_SheetWrite 0x815f53f7
void *mSheetWrite(MSheet *sheet,int row,int col,void *data,int size)
{
    mException(INVALID_POINTER(sheet),EXIT,"invalid input source sheet");
    mException((size<=0)&&(INVALID_POINTER(data)),EXIT,"invalid input sheet element size");
    mException((row>sheet->row)||(row<0),EXIT,"invalid write location");
    if(size<=0) size = strlen((char *)data)+1;
   
    struct HandleSheetCreate *handle0 = (struct HandleSheetCreate *)(((MHandle *)(sheet->handle->data[0]))->handle);
    
    if(row == sheet->row) 
        mSheetRowAppend(sheet,DFLT);
    mException((col>sheet->col[row]),EXIT,"invalid write location");

    if(col<0) col = sheet->col[row];
    if(col == sheet->col[row]) 
        mSheetColAppend(sheet,row,DFLT); 
    
    if(handle0->memory == NULL)handle0->memory = mMemoryCreate(DFLT,DFLT,MORN_HOST);
    sheet->data[row][col] = mMemoryWrite(handle0->memory,data,size);
    
    if((row!=sheet->row-1)&&(col!=sheet->col[row]-1))
    {
        MHandle *hdl=mHandle(sheet,SheetWrite);
        struct HandleSheetWrite *handle = (struct HandleSheetWrite *)(hdl->handle);
        handle->write_size += size;
        
        if(handle->write_size>16384)
        {
            mSheetOperate(sheet,MemoryCollect,handle0->memory);
            MemoryDefrag(handle0->memory);
        }
    }
    return (sheet->data[row][col]);
}
    
struct HandleSheetRead
{
    int row_order;
    int col_order;
};
void endSheetRead(void *info) {}
#define HASH_SheetRead 0x4c826020
void *mSheetRead(MSheet *sheet,int row,int col,void *data,int size)
{
    mException(INVALID_POINTER(sheet),EXIT,"invalid input");
    mException((size <=0)||(INVALID_POINTER(data)),EXIT,"invalid input"); 
    
    MHandle *hdl=mHandle(sheet,SheetRead);
    struct HandleSheetRead *handle = (struct HandleSheetRead *)(hdl->handle);

    if(row<0)
    {
        row = handle->row_order; 
        if(col>=0) 
        {
            mException((col>=sheet->col[row]),EXIT,"invalid input");
            handle->col_order = col;
        }
        else
        {
            if(handle->col_order < sheet->col[row]-1)
            {
                handle->col_order += 1;
                col = handle->col_order;
            }
            else
            {
                if(row == sheet->row-1) col = handle->col_order;
                else
                {
                    handle->row_order += 1;
                    row = handle->row_order;
                    handle->col_order = 0;
                    col = 0;
                }
            }
        }
    }
    else
    {
        mException((row>=sheet->row),EXIT,"invalid input");
        handle->row_order = row;
        if(col>=0) 
        {
            mException((col>=sheet->col[row]),EXIT,"invalid input");
            handle->col_order = col;
        }
        else
        {
            if(handle->col_order < sheet->col[row]-1)
                handle->col_order += 1;
            col = handle->col_order;
        }
    }
    if(data!=NULL)
    {
        if(size>0)memcpy(        data,         sheet->data[row][col],size);
        else      strcpy((char *)data,(char *)(sheet->data[row][col]));
    }
    return sheet->data[row][col];
}

void mSheetElementDelete(MSheet *sheet,int row,int col)
{
    mException(INVALID_POINTER(sheet),EXIT,"invalid input");
    mException((row>=sheet->row),EXIT,"invalid input");
    mException((col>=sheet->col[row]),EXIT,"invalid input");
    memmove(sheet->data[row]+col,sheet->data[row]+col+1,(sheet->col[row]-col-1)*sizeof(void *));
}

void *mSheetElementInsert(MSheet *sheet,int row,int col,void *data,int size)
{
    mSheetWrite(sheet,row,DFLT,data,size);
    void *buff = sheet->data[row][sheet->col[row]-1];
    
    memmove(sheet->data[row]+col+1,sheet->data[row]+col,(sheet->col[row]-col-1)*sizeof(void *));
    sheet->data[row][col] = buff;
    return buff;
}

void mSheetReorder(MSheet *sheet)
{
    mException(INVALID_POINTER(sheet),EXIT,"invalid input source sheet");
    
    int i,j,k;
    void *buff;
    for(j=0;j<sheet->row;j++)
    {
        void **data = sheet->data[j];
        int num = sheet->col[j];
        
        for(i=0;i<num;i++)
        {
            k = mRand(0,num);
            buff = data[i]; data[i] = data[k]; data[k] = buff;
        }
    }
}

inline uint16_t _LookupHash(const void *p,int size)
{
    if(size==1) return ((uint16_t)(*(uint8_t *)p));
    uint16_t *data = (uint16_t *)p;size=size/2;
    int sum=data[0];
    for(int i=1;i<size;i++) sum+=data[i];
    return ((uint16_t)(sum&0x0FFFF));
}

inline int _LookupCompare(const char *mem1,int size1,const char *mem2,int size2)
{
    int d=mem1[0]-mem2[0];if(d) return d;
    for(int i=1;i<MIN(size1,size2);i++) {d=mem1[i]-mem2[i];if(d) return d;}
    return (size1-size2);
}

// inline int _LookupSearch(int **sheet_data,int num,void *key,int key_size)
// {
//     pthread_rwlock_rdlock(&(handle->rwlock));
    
//     int step = (num+1)/4;
//     int n=MAX(num/2,1);

//     int *data = sheet_data[n];
//     int f = _LookupCompare(data+2,data[0],key,key_size);
//     if(f==0) {pthread_rwlock_unlock(&(handle->rwlock)); return n;}
    
//     while(step!=0)
//     {
//         if(f<0) n=n+step;
//         else    n=n-step;
        
//         data = sheet_data[n];
//         f = _Compare(data+2,data[0],key,key_size);
        
//         if(f==0) {pthread_rwlock_unlock(&(handle->rwlock)); return n;}
//         step=step>>1;
//     }
//     // if(
// }

void *mLookupWrite(MSheet *sheet,void *key,int key_size,void *value,int value_size)
{
    mException(INVALID_POINTER(sheet)||INVALID_POINTER(key),EXIT,"invalid input");
    if(key_size  <=0) {key_size  = strlen((char *)key  )  ;} int mkey_size  =((key_size  +7)>>3)*(8/sizeof(int));
    if(value_size<=0) {value_size= strlen((char *)value)+1;} int mvalue_size=((value_size+7)>>3)*(8/sizeof(int));
    
    if(sheet->row!=65536)
    {
        mSheetRowAppend(sheet,65536);
        memset(sheet->col,0,65536*sizeof(int));
    }
    int *data;
    uint16_t hash = _LookupHash(key,key_size);
    int i;for(i=0;i<sheet->col[hash];i++)
    {
        data = (int *)(sheet->data[hash][i]);
        if(_LookupCompare((char *)key,key_size,(char *)(data+2),data[0])<0) break;
    }
    data = (int *)mSheetElementInsert(sheet,hash,i,NULL,(2+mkey_size+mvalue_size)*sizeof(int));
    data[0]=key_size;data[1]=value_size;
    memcpy(data+2,key,key_size);
    if(value!=NULL) memcpy(data+2+mkey_size,value,value_size);
    return (data+2+mkey_size);
}

void *mLookupRead(MSheet *sheet,const void *key,int key_size,void *value,int value_size)
{
    mException(INVALID_POINTER(sheet)||INVALID_POINTER(key),EXIT,"invalid input");
    mException(sheet->row!=65536,EXIT,"invalid input");
    if(key_size  <=0) {key_size  = strlen((char *)key  )  ;} int mkey_size  =((key_size  +7)>>3)*(8/sizeof(int));
    
    uint16_t hash = _LookupHash(key,key_size);
    int *data=NULL;
    int i;for(i=0;i<sheet->col[hash];i++)
    {
        data = (int *)(sheet->data[hash][i]);
        int d=_LookupCompare((char *)key,key_size,(char *)(data+2),data[0]);
        if(d<0) return NULL;
        if(d==0) break;
    }
    if(value!=NULL)
    {
        if(value_size <=0) value_size= data[1];
        else               value_size= MIN(value_size,data[1]);
        memcpy(value,data+2+mkey_size,value_size);
    }
    return (data+2+mkey_size);
}

void mLookupDelete(MSheet *sheet,const void *key,int key_size)
{
    
}


struct HashElement
{
    unsigned int hash;
    int key_size;
    void *key;
    int size;
    void *data;
};
struct HandleHashSheet
{
    int num;
};
void endHashSheet(void *info) {}
#define HASH_HashSheet 0xd4458d30
void HashSheetResize(MSheet *sheet)
{
    MSheet *src=mSheetCreate(sheet->row,sheet->col,sheet->data);
    
    mSheetRowAppend(sheet,sheet->row*2);
    for(int j=0;j<sheet->row;j++) sheet->col[j] = 0;
    int sheet_row = sheet->row-1;
    
    for(int j=0;j<src->row;j++)
        for(int i=0;i<src->col[j];i++)
        {
            struct HashElement *p = (struct HashElement *)(src->data[j][i]);
            int row = p->hash & sheet_row;
            int col = sheet->col[row];
            mSheetColAppend(sheet,row,DFLT);
            
            sheet->data[row][col] = p;
        }
        
    mSheetRelease(src);
}


void *mHashSheetWrite(MSheet *sheet,void *key,int key_size,void *data,int size)
{
    mException(INVALID_POINTER(sheet),EXIT,"invalid input sheet");
    if(key_size<0) key_size = strlen((char *)key);
    if(size <0) size = strlen((char *)data);
    
    MHandle *hdl=mHandle(sheet,HashSheet);
    struct HandleHashSheet *handle = (struct HandleHashSheet *)(hdl->handle);
    if(hdl->valid == 0)
    {
        handle->num = 0;
        mSheetRowAppend(sheet,1024);
        hdl->valid = 1;
    }
    
    int i;
    unsigned int hash = mHash((const char *)key,key_size);
    int row = hash&(sheet->row-1);
    for(i=0;i<sheet->col[row];i++)
    {
        struct HashElement *p = (struct HashElement *)(sheet->data[row][i]);
        if(key_size == p->key_size) if(memcmp(p->key,key,key_size)==0) break;
    }
    int col = i;
    
    int key_size2 = (((key_size+3)>>2)<<2);
    int size2 = (((size+3)>>2)<<2);
    mSheetWrite(sheet,row,col,NULL,sizeof(int)+sizeof(int)+sizeof(int)+key_size2+size2);
    struct HashElement *p = (struct HashElement *)(sheet->data[row][col]);
    p->hash = hash;
    p->key_size = key_size;
    p->size = size;
    p->key = (((int *)p)+3);
    p->data =(((char *)(p->key)) + key_size2);
    memcpy(p->key,key,key_size);
    memcpy(p->data,data,size);
    
    handle->num += 1;
    if(handle->num > sheet->row)
        HashSheetResize(sheet);
    
    return p->data;
}

void *mHashSheetRead(MSheet *sheet,void *key,int key_size,void *data,int size)
{
    unsigned int hash = mHash((const char *)key,key_size);
    int row = hash&(sheet->row-1);
    
    int i;
    struct HashElement *p=NULL;
    for(i=0;i<sheet->col[row];i++)
    {
        p = (struct HashElement *)(sheet->data[row][i]);
        if(key_size == p->key_size) if(memcmp(p->key,key,key_size)==0) break;
    }
    if(i==sheet->col[row])
        return NULL;
    
    if(data != NULL)
    {
        if(size<=0) size = p->size;
        memcpy(data,p->data,size);
    }
    return p->data;
}

void mHashSheetElementDelete(MSheet *sheet,void *key,int key_size)
{
    MHandle *hdl=mHandle(sheet,HashSheet);
    struct HandleHashSheet *handle = (struct HandleHashSheet *)(hdl->handle);
    mException((hdl->valid == 0),EXIT,"invalid input sheet");
        
    unsigned int hash = mHash((const char *)key,key_size);
    int row = hash&(sheet->row-1);
    
    int i;
    struct HashElement *p;
    for(i=0;i<sheet->col[row];i++)
    {
        p = (struct HashElement *)(sheet->data[row][i]);
        if(key_size == p->key_size) if(memcmp(p->key,key,key_size)==0) break;
    }
    if(i==sheet->col[row]) return;
    int col = i;
    
    mSheetElementDelete(sheet,row,col);
    
    handle->num -= 1;
}


