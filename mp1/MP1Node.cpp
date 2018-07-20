/**********************************
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
    initMemberListTable(memberNode);

    return 0;
}

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
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

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

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * where does this MessageHdr come from? data stands for the message
	 */

    //prelimilary step
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

            if (this->memberNode->inited && 
                !this->memberNode->bFailed) {

                //at the first step, would the heartbeat has to be increased?

                /*
                 getting the requester information
                 message structure ---> MsgType, address, number of members, member data (id, port, heartbeat)
                */
                Address requesterAddress;
                memcpy(requesterAddress.addr, data, sizeof(memberNode->addr.addr));     //extract requester's Address from data
                data += sizeof(memberNode->addr.addr);
                size -= sizeof(memberNode->addr.addr);

                long heartbeat;
                memcpy(&heartbeat, data, sizeof(long));   //extract heartbeat from data

                string reqAddStr = (requesterAddress.getAddress());
                int id = getIdFromAddress(reqAddStr);
                short port = getPortFromAddress(reqAddStr);

                //update the member in the membership list
                updateMembershipList(id, port, heartbeat);

                //send membership list to requester
                sendMembershipList(&requesterAddress, JOINREP);

                cout << "...end joinReqHandler." << endl;
            return true;
            }

        case(JOINREP):
        /* over here you need to decode the response (it will be in the 
        form of a character buffer that represents a list of nodes and 
        add the entries to your own list */
            cout << "start joinRepHandler..." << endl;

            // since this is reply
            if (size < (int)(sizeof(memberNode->addr.addr))) {
                return false;
            }

            Address replierAddr;
            memcpy(replierAddr.addr, data, sizeof(memberNode->addr.addr));          //extract replier's Address from data
            data += sizeof(memberNode->addr.addr);
            size -= sizeof(memberNode->addr.addr);

            if (!recvMembershipList(env, data, size, "JOINREP")) {
                return false;
            }
            //???
            this->memberNode->inGroup = true;


            return true;
    }
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {
	/*
	 * Your code goes here
	 */

    return;
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
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: recvMembershipList
 *
 * DESCRIPTION: receive a membership list from another node. Make necessary updates to own membership list.
 */
bool MP1Node::recvMembershipList(void *env, char *data, int size, const char * label) {

    //get the number of members
    long numberOfMembers;

    //???
    memcpy(&numberOfMembers, data, sizeof(long));
    data += sizeof(long);
    size -= sizeof(long);

    //sanity check (log is in log class)
    if (size < (int)(numberOfMembers * (sizeof(int) + sizeof(short) + sizeof(log)))) {
        return false;
    }

    //extract each member from data and update own membership list
    MemberListEntry entry;
    for (int i = 0; i < numberOfMembers; i++) {

        memcpy(&entry.id, data, sizeof(int));
        data += sizeof(int);
        memcpy(&entry.port, data, sizeof(short));
        data += sizeof(short);
        memcpy(&entry.heartbeat, data, sizeof(long));
        data += sizeof(long);
        entry.timestamp = par->getcurrtime();

        int id = entry.id;
        short port = entry.port;
        long heartbeat = entry.heartbeat;

        updateMembershipList(id, port, heartbeat);
    }

    return true;
}




// where should this function be? 
/*

    Takes in a serialized repn of the member list coming from node n and 
    parses it to update your own table, also update the entry of the resp_addr

*/
void MP1Node::updateMembershipList(int id, short port, long heartbeat){
    cout << "updating membership list ..." << endl;

    int sizeOfMemberList = this->memberNode->memberList.size();
    for (int i = 0; i < sizeOfMemberList; i++) {
        if (memberNode->memberList[i].id == id && memberNode->memberList[i].port == port) {
            if (heartbeat > memberNode->memberList[i].getheartbeat()) {
                memberNode->memberList[i].heartbeat = heartbeat;
                memberNode->memberList[i].settimestamp(par->getcurrtime());
            }
            return;
        }
    }

    //if the memberlist does not contain the entry, create a new one and push it into the list
    MemberListEntry entry(id, port, heartbeat, par->getcurrtime());
    memberNode->memberList.push_back(entry);

#ifdef DEBUGLOG
    Address logAddr;
    memcpy(&logAddr.addr[0], &entry.id, sizeof(int));
    memcpy(&logAddr.addr[4], &entry.port, sizeof(short));
    log->logNodeAdd(&memberNode->addr, &logAddr);
#endif


}

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
