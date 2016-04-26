#include <stdio.h>
#include <sys/param.h>
#include "forward_index.h"
#include "tokenize.h"
#include "util/logging.h"


ForwardIndex *NewForwardIndex(t_docId docId, float docScore) {
    
    ForwardIndex *idx = malloc(sizeof(ForwardIndex));
    
    idx->hits = kh_init(32);
    idx->docScore = docScore;
    idx->docId = docId;
    idx->totalFreq = 0;
    idx->maxFreq = 0;
    
    return idx;
}

void ForwardIndexFree(ForwardIndex *idx) {
    kh_destroy(32, idx->hits);
    free(idx);
    //TODO: check if we need to free each entry separately
}


void ForwardIndex_NormalizeFreq(ForwardIndex *idx, ForwardIndexEntry *e) {
    
    printf("Pre norm: %d\n", e->freq);
    e->freq = 1+(u_int16_t)((FREQ_QUANTIZE_FACTOR-1) * ((float)e->freq/(float)idx->maxFreq));
    printf("Post norm: %d\n", e->freq);

}

int forwardIndexTokenFunc(void *ctx, Token t) {
    ForwardIndex *idx = ctx;
    
    ForwardIndexEntry *h = NULL;
    // Retrieve the value for key "apple"
    khiter_t k = kh_get(32, idx->hits, t.s);  // first have to get ieter
    if (k == kh_end(idx->hits)) {  // k will be equal to kh_end if key not present
        h = calloc(1, sizeof(ForwardIndexEntry));
        h->docId = idx->docId;
        h->term = t.s;
        h->vw = NewVarintVectorWriter(4);

        int ret;
        k = kh_put(32, idx->hits, t.s, &ret);
        kh_value(idx->hits, k) = h;
    } else {
        h = kh_val(idx->hits, k);
    }

    h->flags |= t.fieldId;
    h->freq += t.score;
    idx->totalFreq += t.score;

    idx->maxFreq = MAX(h->freq, idx->maxFreq);
    VVW_Write(h->vw, t.pos);
    LG_DEBUG("%d) %s, token freq: %d total freq: %d\n", t.pos,t.s, h->freq, idx->totalFreq);
    return 0;
    
}


ForwardIndexIterator ForwardIndex_Iterate(ForwardIndex *i) {
    ForwardIndexIterator iter;
    iter.idx = i;
    iter.k = kh_begin(i->hits);
    
    return iter;
}

ForwardIndexEntry *ForwardIndexIterator_Next(ForwardIndexIterator *iter) {
     
   if (iter->k == kh_end(iter->idx->hits)) {
       return NULL;
   }
   if (kh_exist(iter->idx->hits, iter->k)) {
         ForwardIndexEntry *entry = kh_value(iter->idx->hits, iter->k);
         iter->k++;
         return entry;
      
   } else {
       iter->k++;
       return ForwardIndexIterator_Next(iter);
   }
   return NULL;
}