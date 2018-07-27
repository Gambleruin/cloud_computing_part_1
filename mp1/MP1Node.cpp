`/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/
#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */
void MP1Node::printMessage(string callr_fn, Address *sendr, Address *recvr, 
                            MessageHdr *msg, int size)
{
    MsgTypes msgType;
    //char *msg_chr;
    
    cout << "<bbi>[" << this->par->getcurrtime() << "]in " << callr_fn << " of MP1Node:" << this->memberNode->addr.getAddress();
    memcpy(&msgType, msg, sizeof(MsgTypes));
    //msg_chr = (char *)msg;
    
    switch(msgType) {
        case(JOINREQ): 
            cout << " JOINREQ"; 
            break;
  
        case(JOINREP): 
            cout << " JOINREP"; 
            break;

        case(MMBRTBL): 
            cout << " MMBRTBL"; 
            break;

        case(DUMMYLASTMSGTYPE): 
            cout << "DUMMYLASTMSGTYPE" << " "; 
            break;
        
        default: 
            cout << "UNKNOWN";

    }
    cout << " from=" << sendr->getAddress();
    cout << " to=" << recvr->getAddress();
    cout << endl;
}

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;

    // mark node failure to be false
    memberNode->mark_fail =0;                      // whether the node has been marked for fail
    memberNode->mark_del =0;                       //whether the node has been marked for deletion

    initMemberListTable(memberNode);
    return 0;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
    memberNode->memberList.clear();
    // 1) invoke the deconstrutor for every MemberListEntry
    // 2) size == 0 (the vector contained the actual objects).
}

// later to be used
int MP1Node::getIdFromAddress(string address) {
    size_t pos = address.find(":");
    int id = stoi(address.substr(0, pos));
    return id;
}

short MP1Node::getPortFromAddress(string address) {
    size_t pos = address.find(":");
    short port = (short)stoi(address.substr(pos + 1, address.size()-pos-1));
    return port;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        /*
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
        */

        size_t msgsize = sizeof(messagehdr) + sizeof(address) + sizeof(int)+ sizeof(MemberEntry)*(node->numMemberEntries);
        msg=malloc(msgsize);
        createJoinReq(node,msg);

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }
    return 1;

}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
    return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator (what does this coodinator mean exactly)
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/* This function copies a serialized repn of the memberlist into the 
   buffer pointed to by buffer*/
void MP1Node::serializeMemberTable(member* self, char* buffer){
    memcpy(buffer,&self->numMemberEntries,sizeof(int)); 
    memcpy(buffer+sizeof(int),self->memberList,sizeof(MemberEntry)*self->numMemberEntries);     
}

/* create JOINREQ message: format of data is msghdr|myaddr|listlen|list */
void MP1Node::createJoinReq(member* self, char*buffer){
    ((messagehdr*)buffer)->msgtype=JOINREQ;
    memcpy(buffer+sizeof(messagehdr), &self->addr, sizeof(address));
    serializeMemberTable(self,buffer+sizeof(address)+sizeof(messagehdr));
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
    /*
     * where does this MessageHdr come from? data stands for the message
     */

    //check message size
    if(size < (int)sizeof(MessageHdr)) {

#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Message received with size less than MessageHdr. Ignored.");
#endif
        return false;
    }

    MessageHdr* messageHdr = (MessageHdr*) data;
    MsgTypes msgType = messageHdr->msgType;

    //between case analysis
    switch(msgType){
        case(JOINREQ):
        /* 
            Do the following things in here 
            1. The JOINREQ is received by the introducer
            2. Update its own table
            3. It responds back with a current copy of the membership table.
        */
            cout << "start joinReqHandler..." << endl;

            //this->memberNode is node that is receving request
            if (this->memberNode->inited && 
                !this->memberNode->bFailed) {
                /*
                 getting the requester information
                 message structure ---> MsgType, address, number of members, member data (id, port, heartbeat)
                */
                member *self = (member*) env; //own address
            /*
                Address req_addr;
                memcpy(req_addr.addr, data, sizeof(memberNode->addr.addr));     //extract requester's Address from data
            */
                Address* req_addr = (address*)data; //extract address of the requesting node

                //???
                data += sizeof(memberNode->addr.addr);
                size -= sizeof(memberNode->addr.addr);

                long heartbeat;
                memcpy(&heartbeat, data, sizeof(long));   //extract heartbeat from data (this is not needed anymore)

                // extra methods
                string reqAddStr = (requesterAddress.getAddress());
                int id = getIdFromAddress(reqAddStr);
                short port = getPortFromAddress(reqAddStr);

                /* data now corresponds to the actual content of the message */
                /* add the node to the local table. */
                updateNodeTable(self,req_addr,data,size);

                // responds back with a current copy of the membership table. (updated membership table displays of any updated heartbeats)
                /* build your response ( your copy of the membership table */
                size_t msgsize = sizeof(messagehdr)+sizeof(address)+sizeof(int)+sizeof(MemberEntry)*(self->numMemberEntries);
                char * msg = malloc(msgsize);
                ((messagehdr*) msg)->msgtype = JOINREP;
                memcpy(msg+sizeof(messagehdr),&self->addr,sizeof(address));
                // write join notification
                serializeMemberTable(self,msg+sizeof(messagehdr)+sizeof(address));

                /* send your respose */
                MPp2psend(&self->addr,req_addr,(char*)msg,msgsize);
                free(msg);
    
            return true;
            }

        case(JOINREP):
        /* over here you need to decode the response (it will be in the 
        form of a character buffer that represents a list of nodes and 
        add the entries to your own list */
            cout << "start joinRepHandler..." << endl;

            if (size < (int)(sizeof(memberNode->addr.addr))) {
                return false;
            }
/*
            Address replierAddr;
            memcpy(replierAddr.addr, data, sizeof(memberNode->addr.addr));          //extract replier's Address from data
            data += sizeof(memberNode->addr.addr);
            size -= sizeof(memberNode->addr.addr);

            if (!recvMembershipList(env, data, size, "JOINREP")) {
                return false;
            }
*/
            char addr_str[20];
            member *self = (member*) env;
            address* resp_addr = (address*)data;
            data = (char*)(resp_addr+1); 
            size -= sizeof(address);
            
            self->ingroup =true;
            //this->memberNode->inGroup = true;
            return true;

        //process gossip message
        case(GOSSIP):
        
            cout << "start to process Gossip... on receving" << endl;

            if (size < (int)(sizeof(memberNode->addr.addr))) {
                return false;
            }

            char addr_str[20];
            member *self = (member*) env;
            address* resp_addr = (address*)data;
            data = (char*)(resp_addr+1); 
            size -= sizeof(address);
            print_address(addr_str,resp_addr);
    
            LOG(&self->addr,"Received a GOSSIP message from %s",addr_str);

            /* data now points to the actual message contents */
            if(size>0) 
                updateNodeTable(self,resp_addr,data,size);
            else{
                LOG(&self->addr,"Join response is empty!");
            }
            //what should be returned
            return;

        case(DUMMYLASTMSGTYPE): 
            break;
            
        default: 
            return false;      
    }
}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
    return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();
    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }
    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/* 
Executed periodically for each member. 
Performs necessary periodic operations. 
Called by nodeloop(). 
Check if any node hasn't responded within a timeout period and then delete
 *              the nodes
 *              Propagate your membership list
*/
void MP1Node::nodeloopOps(member *node){
    cout << "start nodeLoopOps..." << endl;

    // Over here, update the heartbeat counter and keep yourself alive;
    keepSelfAlive(node);
    /*gossip your table to a random member in your list */
    sendGossip(node);
    /*check for expired entries in your table */
    checkNodeTable(node);
    return;
}

/* This function updates your own hearbeat counter */
void MP1Node::keepSelfAlive(member *node){

    // node pointing to the first element is itself? 
    node->memberList[0].last_hb++;
    node->memberList[0].last_local_timestamp = getcurrtime();
    node->memberList[0].mark_fail = 0;
    node->memberList[0].mark_del = 0;
}

void MP1Node::sendGossip(member* node){

    // the original version
    // no gossip needed
    if(node->numMemberEntries==1) 
        return; 

    //choose a candidate
    int maxtries = 3;
    int randnode;
    while(maxtries--){
        randnode = 1+rand()%(self->numMemberEntries-1);
        if(self->memberList[randnode].mark_fail==1) 
            continue;
        else break;
    }

    MemberListEntry &entry = memberNode->memberList[randnode];
    //check if that node has failed before sending member list to it
    if (par->getcurrtime() - entry.timestamp > TFAIL) {
        return;
    }

    //send message
    //????

    char debug_buffer[50];
    print_address(debug_buffer,&self->memberList[randnode].addr);
    //LOG(&self->addr,"Sending a GOSSIP message to %s",debug_buffer);
    address* send_addr = &self->memberList[randnode].addr;
    size_t messagesize = sizeof(messagehdr) + sizeof(address) + sizeof(int) + sizeof(MemberEntry)*self->numMemberEntries;
    char* msg = malloc(messagesize);
    ((messagehdr*)msg)->msgtype = GOSSIP;
    memcpy(msg+sizeof(messagehdr),&self->addr,sizeof(address));
    serializeMemberTable(self,msg+sizeof(messagehdr)+sizeof(address));
    MPp2psend(&self->addr,send_addr, (char *)msg, messagesize);
    free(msg);

    /*
    Address toAddr;
    memcpy(&toAddr.addr[0], &entry.id, sizeof(int));
    memcpy(&toAddr.addr[4], &entry.port, sizeof(short));
    this->sendMembershipList(&toAddr, HEARTBEATREQ);
    */
    return;
}

/* This function checks the node table at a particular node and 
   marks up any deleted or failed entries */
void MP1Node::checkNodeTable(member* self){
    int i;
    char debug_buffer[50];
    for(i=1;i<self->numMemberEntries;++i){

        if( !self->memberList[i].mark_fail ){ 
            if( (getcurrtime()-self->memberList[i].last_local_timestamp ) > self->tfail){
            /* tfail timer has expired , mark node as failed */
                int64_t oldts = self->memberList[i].last_local_timestamp;
                self->memberList[i].mark_fail=1;
                self->memberList[i].last_local_timestamp = getcurrtime();
                print_address(debug_buffer,&self->memberList[i].addr);
                LOG(&self->addr,"\t\tMarking node %s as Failed on %d , entry last updated at %d",debug_buffer,getcurrtime(),oldts);
            }
        }else{
            /* tdelete timer has expired, mark node for deletion */
            if( (getcurrtime()-self->memberList[i].last_local_timestamp ) > self->tdelete){
                //swap it with the last member in the list to delete it.
                int64_t oldts = self->memberList[i].last_local_timestamp;
                logNodeRemove(&self->addr,&self->memberList[i].addr); 
                print_address(debug_buffer,&self->memberList[i].addr);

                LOG(&self->addr,"\t\tMarking node %s as Deleted on %d , entry last updated at %d",debug_buffer,getcurrtime(),oldts);
                self->memberList[i] = self->memberList[self->numMemberEntries-1];
                self->numMemberEntries--;
            }
        }
    }
}

/*
    Takes in a serialized repn of the member list coming from node n and 
    parses it to update your own table, also update the entry of the resp_addr
*/
void MP1Node::updateNodeTable(member* self, address* other_addr,char* data,int datasize){
    
    char debug_buffer[100];
    int i,j;
    int* otherListSize = (int*)data;
    if((*otherListSize)*sizeof(MemberEntry) < (datasize-sizeof(int))){
        LOG(&self->addr,"Bad Packet");
        return ;
    }

    /*iterate over their list */
    MemberEntry* otherList = (MemberEntry*)(otherListSize+1);
    for(j=0;j<*otherListSize;++j){
        int updateMade = 0;
        
        /* ignore this entry if this entry is not reliable */
        if(otherList[j].mark_fail) 
            continue;

        /* iterate over my list */
        for(i=1;i<self->numMemberEntries;++i){
            if( memcmp(&otherList[j].addr,&self->memberList[i].addr,sizeof(address))==0) {
                updateMade = 1;
                /* DOUBT: if the process has been marked as failed locally and we still have a good entry from that process,
                   then can i mark this process as alive ? For now , I am doing so */   
  
                if(self->memberList[i].last_hb>=otherList[j].last_hb) break; //no need to update
                else{
                    if(!self->memberList[i].mark_fail){
                        //update the heartbeat of the process and add a local timestamp
                        int64_t oldhb=self->memberList[i].last_hb;
                        self->memberList[i].last_hb = otherList[j].last_hb;
                        self->memberList[i].last_local_timestamp = getcurrtime();   
                        print_address(debug_buffer, &self->memberList[i].addr);

                        //LOG(&self->addr,"\t\tUpdated the entry for %s with hb_new %d vs %d hb_old\n",debug_buffer,self->memberList[i].last_hb,oldhb);        
                    }else{
                        int64_t oldhb=self->memberList[i].last_hb;
                        self->memberList[i].last_hb = otherList[j].last_hb;
                        self->memberList[i].last_local_timestamp = getcurrtime();   
                        self->memberList[i].mark_fail=0; //reverse your decision as you got a greater hb
                        printf("reverse your decision as you got a greater hb");
                        print_address(debug_buffer, &self->memberList[i].addr);
                        //LOG(&self->addr,"\t\tReviving the node at %s with hb_new %d vs  %d hb_old\n",debug_buffer,self->memberList[i].last_hb,oldhb);
                    }
                }
            }
        }

        if(!updateMade && memcmp(&otherList[j].addr,&self->memberList[0].addr,sizeof(address))!=0){
            //this is a new node. append it at the end of the list
            if(self->numMemberEntries<MAX_NNB){ 
                self->memberList[self->numMemberEntries] = otherList[j];
                self->memberList[self->numMemberEntries].last_local_timestamp = getcurrtime(); //stamp it with a local timestamp
                print_address(debug_buffer,&self->memberList[self->numMemberEntries].addr);
                //LOG(&self->addr,"\t\tAdded the entry for %s with hb %d",debug_buffer,self->memberList[self->numMemberEntries].last_hb);        
                self->numMemberEntries++;
#ifdef DEBUGLOG
                logNodeAdd(&self->addr,&otherList[j].addr); 
#endif
            }
            else        
                LOG(&self->addr,"Membership list overflow!");
        }
    }    
} 
/*

    //completess and accuracy tests fail when this executed of par->getcurrtime() <= 3
    if (par->getcurrtime() > 3 && memberNode->memberList.size() > 1) {

        //update own node's heartbeat and timestamp
        int id = getIdFromAddress(memberNode->addr.getAddress());
        short port = getPortFromAddress(memberNode->addr.getAddress());
        for (vector<MemberListEntry>::iterator entry = memberNode->memberList.begin();
             entry != memberNode->memberList.end(); entry++) {
            if (entry->id == id && entry->port == port) {
                entry->settimestamp(par->getcurrtime());
                entry->heartbeat = entry->heartbeat + 1;
                break;
            }
        }

*/

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
