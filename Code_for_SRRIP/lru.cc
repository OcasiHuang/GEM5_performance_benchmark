/*
 * Copyright (c) 2012-2013 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Erik Hallnor
 */

/**
 * @file
 * Definitions of a LRU tag store.
 */

#include "mem/cache/tags/lru.hh"

#include "debug/CacheRepl.hh"
#include "mem/cache/base.hh"

// since we are implementing 2-bit SRRIP-HP, we set maximum value of rrpv to 3 and initial value of rrpv to 2
#define RRPV_max 3
#define RRPV_init 2

LRU::LRU(const Params *p)
    : BaseSetAssoc(p)
{
//    if(p->RRPV_max == 0){
//        fatal("max RRPV should be at least 1.\n");
//    }
}

CacheBlk*
LRU::accessBlock(Addr addr, bool is_secure, Cycles &lat)
{
    CacheBlk *blk = BaseSetAssoc::accessBlock(addr, is_secure, lat);

    if (blk != nullptr) {
        // move this block to head of the MRU list
        //sets[blk->set].moveToHead(blk);
    	//sets[blk->set].moveToTail(blk);

	//set rrpv to 0 when access again
	if(blk->rrpv > 0){
            blk->rrpv = 0;
	}
        //DPRINTF(CacheRepl, "set %x: moving blk %x (%s) to MRU\n",
        //        blk->set, regenerateBlkAddr(blk->tag, blk->set),
        //        is_secure ? "s" : "ns");
        DPRINTF(CacheRepl, "set %x: set blk %x (%s) RRPV value to 0\n",
                blk->set, regenerateBlkAddr(blk->tag, blk->set),
                is_secure ? "s" : "ns");
    }

    return blk;
}

CacheBlk*
LRU::findVictim(Addr addr)
{
    int set = extractSet(addr);//Calculate the set indexe from the address
    // grab a replacement candidate
    BlkType *blk = nullptr;
    // for (int i = assoc - 1; i >= 0; i--) {
    while(1){
        int flag = 0; 
        // find leftmost element with rrpv = RRPV_max, if victiom is found, break and return the victim blk
    	for (int i = 0; i <= assoc - 1; i++) { 
        	BlkType *b = sets[set].blks[i];
        	if (b->way < allocAssoc && b->rrpv == RRPV_max) {
            		blk = b;
			flag = 1;
            		break;
        	}
    	}
	if(flag) break;
	// if no rrpv reaches RRPV_max, all rrpv plus one
    	for (int i = 0; i <= assoc - 1; i++) { 
        	BlkType *b = sets[set].blks[i];
        	if (b->way < allocAssoc) {
			b->rrpv++;
		}
	}
    }
    assert(!blk || blk->way < allocAssoc || blk->rrpv == RRPV_max);

    if (blk && blk->isValid()) {
        DPRINTF(CacheRepl, "set %x: selecting blk %x for replacement\n",
                set, regenerateBlkAddr(blk->tag, set));
    }

    return blk;
}
//blks[0] = head
void
LRU::insertBlock(PacketPtr pkt, BlkType *blk)
{
    BaseSetAssoc::insertBlock(pkt, blk);

    int set = extractSet(pkt->getAddr());
    //sets[set].moveToHead(blk);
    // to ensure that block is insert at tail, not head
    sets[set].moveToTail(blk);
    // set initial rrpv value
    blk->rrpv = RRPV_init;
}

void
LRU::invalidate(CacheBlk *blk)
{
    BaseSetAssoc::invalidate(blk);

    // should be evicted before valid blocks
    int set = blk->set;
    // to ensure that block is insert at tail, not head
    sets[set].moveToTail(blk);
    // set initial rrpv value
    blk->rrpv = RRPV_max;
}

LRU*
LRUParams::create()
{
    return new LRU(this);
}
