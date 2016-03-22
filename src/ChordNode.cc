/*
 * ChordNode.cc
 *  Created on: March 3, 2016
 *
 *      Author: Aniruddha Gokhale
 *      Class:  CS6381 - Distributed Systems Principles
 *      Institution: Vanderbilt University
 *
 */

#include <fstream>
#include <string>
using namespace std;

#include "inet/common/INETEndians.h"  // for host/network byte ordering
#include "inet/networklayer/common/L3AddressResolver.h"

#include "ChordP2PMsg_m.h"     // generated header from the message file
#include "ChordNode.h"         // our header
#include "Helper.h"

using namespace inet;

// register the module with Omnet++
Define_Module(ChordNode);

// constructor and destructors
ChordNode::ChordNode (void)
    : myID_ (-1),
      localAddress_ (),
      localPort_ (10000),
      finger_table_size_ (0),
      nodeList_ (),
      ft_ (NULL),
      socket_ (nullptr),
      socketMap_ (),
      callerID ()
      //socketStackMap_ ()
{
    // nothing
}

ChordNode::~ChordNode()
{
    // nothing
}

/* implement the three required methods */

// the initialize method. We let the Chord node initialize itself in 2 stages.
// In the first phase (stage 4). Once that is done, then we initialize our finger table
void ChordNode::initialize (int stage)
{
    cSimpleModule::initialize (stage); // let the base class handle initialization in each stage

    if (stage != inet::NUM_INIT_STAGES+1)
        return;

    // I am using the logic that let the first few stages be taken by initializing the lower
    // layers of the TCP/IP stack. Then we first initialize ourselves and create a socket
    // for ourselves. Thereafter, we initialize our finger table and open connections
    // to our finger nodes

    // obtain the values of parameters
    this->localPort_ = this->par ("localPort").longValue ();
    this->finger_table_size_ = helper->num_bits ();

    // get the node list
    helper->chord_node_list (this->nodeList_);

    // we use our simulation assigned ID modulo the number of
    // chord nodes as an index into the node array and assign ourselves that ID. But
    // what I have observed is that although the IDs are unique, their modulo
    // may result in same value. So I am using a hack by combinining my ID plus
    // parent ID and hoping that it gives a unique number. Somehow we need a unique
    // number

    this->myID_
    = this->nodeList_[this->getParentModule()->getId () % helper->num_chord_nodes()];

    // To retrieve our IP address, we ask the resolver to get the underlying IP address
    // associated with the host on which this application is running. That host is found
    // by accessing our parent module.
    L3AddressResolver resolver;
    this->localAddress_ = resolver.resolve (this->getParentModule()->getFullName());

    // now register ourselves with helper database
    helper->register_node (this->myID_, this->localAddress_);

    // allocate space for our finger table
    this->ft_ = new ChordNode::Fingertable [this->finger_table_size_];
    if (!this->ft_) {
        throw cRuntimeError("ChordNode::initialize -- no memory for finger table");
        return;
    }

    EV << "=== ChordNode::initialize (stage " << stage << ")" << endl
            << "\tmyID_ = " << this->myID_ << endl
            << "\tlocalAddess = " << this->localAddress_.str () << endl
            << "\tlocalPort = " << this->localPort_ << endl
            << "\tm = " << this->finger_table_size_ << endl;

    // we start a timer so that we can initialize our socket
    cMessage *timer_msg = new cMessage ("init_socket", 0);
    simtime_t time_at = simTime () + exponential (0.001);
    EV << "=== ChordNode::initialize -- scheduling timer to init socket for "
            << time_at << endl;
    this->scheduleAt (time_at, timer_msg);
    setStatusString ("Timer for Socket Initialization");
}

/** the all serving handle message method */
void ChordNode::handleMessage (cMessage *msg)
{
    EV << "=== ChordNode::handleMessage " << this->myID_
       << " received handleMessage message"
       << endl;

    // check if this was a self generated message, such as a timeout to wake us
    // up to initialize ourselves
    if (msg->isSelfMessage ()) {
        this->handleTimer (msg);
    } else {
        // let the socket class process the message and make a call back on the
        // appropriate method. But note that we need to determine which socket
        // must handle this message: it could be connection establishment in
        // our passive role or it could be an ack to our active conn establishment
        // or it could be a lookup request from a client (or relayed by our finger).
        //
        // so first look up the socket in the map
        TCPSocket *socket = this->socketMap_.findSocketFor (msg);
        if (!socket) {
            // no such socket was found. This means that an incoming connection
            // establishment message has arrived. So we must open a socket to handle
            // the communication with that entity (which could be a client or a finger node)

            // make sure first that we are dealing with a TCP command
            TCPCommand *cmd = dynamic_cast<TCPCommand *>(msg->getControlInfo());
            if (!cmd) {
                throw cRuntimeError("ChordNode::handleMessage: no TCPCommand control info in message (not from TCP?)");
            } else {
                EV << "=== ChordNode:: " << this->myID_
                   << " **No socket yet ** for connID: "
                   << cmd->getConnId ()
                   << ", setting transfer mode to "
                   << this->socket_->getDataTransferMode()
                   << "===" << endl;

                // notice that we must use the other constructor of TCPSocket
                // so that it will use the underlying connID that was created
                // after an incoming connection establishment message
                TCPSocket *new_socket = new TCPSocket (msg);

                // set callback object
                new_socket->setCallbackObject (this, new_socket);

                // do not forget to set the outgoing gate
                new_socket->setOutputGate (gate ("tcpOut"));

                // another thing I learned the hard way is that we must set up
                // the data transfer mode for this new socket
                new_socket->setDataTransferMode (this->socket_->getDataTransferMode ());

                // now save this socket in our map
                this->socketMap_.addSocket (new_socket);

                // process the message on this socket
                new_socket->processMessage (msg);
            }
        } else {
            // let the socket that was found in the map process the message
            socket->processMessage (msg);
        }
    }
}

/** this method is provided to clean things up when the simulation stops */
void ChordNode::finish ()
{
    EV << "=== ChordNode::finish called" << endl;

    // cleanup all the sockets
    this->socketMap_.deleteSockets ();

    std::string modulePath = getFullPath();
}

/** handle the timeout method */
void ChordNode::handleTimer (cMessage *msg)
{
    // The way we have programmed this, we can get two kinds of timer expiry:
    // first is when we must create a listening socket (when kind == 0)
    // second is when we must create our finger table (when kind == 1)
    
    if (msg->getKind () == 0) {
        // this is a init_socket time out
        EV << "=== ChordNode::handleTimer for Node ID: " << this->myID_
           << " being kickstarted to initialize socket ===" << endl;
        setStatusString ("socket init");

        // clean up the timer msg
        delete msg;

        // now initialize the listening socket
        
        // create a new socket for the listening role.
        this->socket_ = new TCPSocket ();
        if (!this->socket_)
            throw cRuntimeError("ChordNode::initialize: no memory for socket");

        // message transfer
        this->socket_->setDataTransferMode (TCP_TRANSFER_OBJECT);

        // In the server role, we bind ourselves to the well-defined port and IP
        // address on which we listen for incoming connections.
        this->socket_->bind (this->localAddress_, this->localPort_);

        // register ourselves as the callback object. Note the second param is
        // whatever pointer we want to send that will help us when the callback
        // happens. So we send a pointer to the socket itself so it is readily
        // available to us
        this->socket_->setCallbackObject (this, this->socket_);

        // do not forget to set the outgoing gate
        this->socket_->setOutputGate (gate ("tcpOut"));

        // now save this socket in our map
        this->socketMap_.addSocket (this->socket_);

        // now listen for incoming connections.  This version is the forking
        // version where upon every new incoming connection, a new socket is created.
        this->socket_->listen ();

        setStatusString ("passively waiting");

        // we start a timer so that we can initialize our socket
        msg = new cMessage ("init_finger", 1);
        simtime_t time_at = simTime () + exponential (0.05);
        EV << "=== ChordNode::handle_timer -- scheduling timer to init finger table for "
               << time_at << endl;

        this->scheduleAt (time_at, msg);
        setStatusString ("Timer for Finger table");

    } else if (msg->getKind () == 1) {
        // this is a initialize finger table time out
        EV << "=== ChordNode::handleTimer for NodeID: " << this->myID_
           << " being kickstarted to initialize finger table ===" << endl;

        // clean up the timer msg
        delete msg;

        // call a helper method that will fill up the finger table for this node
        this->init_finger_table ();

        setStatusString ("finger table init");

    } else {
        // unknown timer
        EV << "=== ChordNode::handleTimer " << this->myID_ << ": unknown timer" << endl;
    }
}

/*************************************************/
/** implement all the callback interface methods */
/*************************************************/

void ChordNode::socketEstablished (int connID, void *yourPtr)
{
    EV << "=== ChordNode::socketEstablished: " << this->myID_
       << " received socketEstablished message on connID "
       << connID << " ===" << endl;

    setStatusString("ConnectionEstablished");
}

/** handle incoming data, which ought to be a request */
void ChordNode::socketDataArrived(int connID, void *yourPtr, cPacket *msg, bool)
{
    // debugging output
    setStatusString ("Request");
    EV << "=== ChordNode::socketDataArrived: " << this->myID_
       << " received socketDataArrived message. on connID: " << connID
       << " of byte length = " << msg->getByteLength ()
       << " ===" << endl; 

    // first cast to the socket on which this message was received
    TCPSocket *socket = static_cast<TCPSocket *> (yourPtr);
    if (!socket) {
        throw cRuntimeError("ChordNode::socketDataArrived -- not a socket");
        return;
    }

    /* @@@ FILL IN @@@ */

    // incoming msg has to be a lookup request either coming directly
    // from a client or from another chord node. Or it can be a response
    // from another finger that we must relayed back to client. We must cast
    // the msg to the appropriate type and take actions.


}

void ChordNode::socketPeerClosed (int connID, void *yourPtr)
{
    // the peer in this case is the client who has made the
    // lookup request to us, which has closed its connection.
    // So we proceed to cleanup our end of the connection by
    // cleanup up the socket.
    EV << "=== ChordNode::socketPeerClosed: " << this->myID_
       << " received socketPeerClosed message"
       << endl; 
    EV << "client closed for connID = " << connID << endl;

    // we should close the socket corresponding to the peer and delete from socket map
    TCPSocket *socket = static_cast<TCPSocket *> (yourPtr);
    if (!socket) {
        throw cRuntimeError("ChordNode::socketPeerClosed -- not a socket");
        return;
    }

    if (socket->getState() == TCPSocket::PEER_CLOSED) {
        socket->close ();
    }
}

void ChordNode::socketClosed (int connID, void *yourPtr)
{
    EV << "=== ChordNode::socketClosed: " << this->myID_
       << " received socketClosed message"
       << endl; 
    EV << "connection closed\n";
    setStatusString("closed");

    // we are closed :-) so we must remove ourselves from the socket map
    // we should close the socket corresponding to the peer and delete from
    // socket map 
    TCPSocket *socket = static_cast<TCPSocket *> (yourPtr);
    if (!socket) {
        throw cRuntimeError("ChordNode::socketClosed -- not a socket");
        return;
    }

    // remove from socket map and delete it
    this->socketMap_.removeSocket (socket);
    delete socket;
}

void ChordNode::socketFailure (int, void *yourPtr, int code)
{
    EV << "=== ChordNode::socketFailure " << this->myID_
       << " received socketFailure message"
       << endl; 
    // subclasses may override this function, and add code try to reconnect
    // after a delay. 
    EV << "connection broken\n";
    setStatusString("broken");

    TCPSocket *socket = static_cast<TCPSocket *> (yourPtr);
    if (!socket) {
        throw cRuntimeError("ChordNode::socketFailure -- not a socket");
        return;
    }

    // remove from socket map and delete it
    this->socketMap_.removeSocket (socket);
    delete socket;
}

/**********************************************************************/
/**           helper methods                                          */
/**********************************************************************/

//@{
/** build the finger table for this node */
void ChordNode::init_finger_table ()
{
    //* @@@ FILL IN @@@ */
    // write the logic to fill up the finger table

    // Get size of finger table and generate it.
    int m = this->finger_table_size_;
    Fingertable table[m];
    this->ft_ = table;
    for(int i=0; i<m; i++){
        // Use mod for situation when it exceeds pow(2, m)
        int id = ((this->myID_ + (int)pow(2, i)) % (int)pow(2, m));
        int suc = this->successor(id);
        (this->ft_ + i)->fingerID = suc;
        (this->ft_ + i)->socket = this->connect(suc);
    }
}

// connect to our finger node
inet::TCPSocket *ChordNode::connect (int fingerID)
{
    //* @@@ FILL IN @@@ */
    // this method is to be used when initializing the finger table where we
    // connect to each of our fingers. Note the parameter is
    // the ID. So use the helper class' method to get the IP addr of the node
    // corresponding to this node ID. All other steps are similar to how
    // we create a new socket. Do not forget to use this class as the callback
    // object.
    EV << "=== ChordNode::connect NodeID: " << this->myID_
       << " connect to the chord node with ID" << fingerID << endl;

    // Create a new socket and connect to it from local socket
    inet::L3Address addr = helper->lookup_node(fingerID);
    inet::TCPSocket *new_socket = new TCPSocket ();

    // Delete existing socket
    if (this->socket_)
        delete this->socket_;

    this->socket_ = new_socket;
    this->socket_->setDataTransferMode (TCP_TRANSFER_OBJECT);
    this->socket_->setOutputGate (gate ("tcpOut"));
    this->socket_->setCallbackObject(this, NULL);
    this->socket_->connect(addr, this->localPort_);

    return this->socket_;
}

/** serve the incoming lookup request */
void ChordNode::serve_lookup (Lookup_Req *req, inet::TCPSocket *socket)
{
    /* @@@ FILL IN WITH REAL CHORD CODE. @@@ */
    // You have to handle 2 cases: the key is with you in which case fill up
    // the response packet and send back. Or, using the chord algo, send the req
    // to the next node. Do not forget to preserve the state because now you
    // become some intermediary who must relay the response back.

    // Make a record who send me request
    this->callerID = std::atoi(req->getSender());
    // Convert integer to char array.
    const char* id  = std::to_string(this->myID_).c_str();
    // Find key and return response
    int key = req->getKey();
    if(key == this->myID_){
        Lookup_Resp *resp = new Lookup_Resp();
        resp->setKey(key);
        resp->setSender(id);
        // Get responder size, which contains a path of intermediate nodes.
        int responder_size = resp->getResponderArraySize();
        resp->setResponderArraySize(responder_size + 1);
        resp->setResponder(responder_size, id);
        // Send it back it caller
        this->connect(this->callerID);
        this->socket_->send(resp);
    }
    // Don't match and pass request to next successor in finger table.
    else{
        int m = this->finger_table_size_;
        for(int i=0; i<m; i++){
            // Connect and pass request to every node in finger table
            this->connect((this->ft_ + i)->fingerID);
            this->socket_->send(req); // Connect socket and relay request.
        }
    }
 }

/** relay the response up the chain */
void ChordNode::relay_resp (Lookup_Resp *resp)
{
    /* @@@ FILL IN  @@@ */
    // Recall that we may be an intermediate chord node who is receiving reply
    // from a downstream chord node, and must relay is to whoever called us.
    // To do that, we must retrieve the connection state we had saved corresponding to
    // this chain of request/reply, and use that to send the response upstream.
    // Do not forget to include ourselves in the chain.

    const char* id  = std::to_string(this->myID_).c_str();
    int responder_size = resp->getResponderArraySize();
    resp->setResponderArraySize(responder_size + 1);
    resp->setResponder(responder_size, id);

    // Connect to caller socket and send response
    this->connect(this->callerID);
    this->socket_->send(resp);
}

// find the successor node
int ChordNode::successor (int id)
{
    /* @@@ FILL IN  @@@ */

    // id is the key whose successor is to be found. We search in the nodeList_.
    // successor is that immediate node which is given by the condition
    // id <= node. So we start checking from the beginning. If we reach the end, then
    // the first node in the list is the successor because we wrap around.

    // Sort node list in accending order
    std::sort(this->nodeList_.begin(), this->nodeList_.end(), [](int a, int b) {
        return (a < b);
    });
    for(int i=0; i<this->nodeList_.size(); i++){
        int value = this->nodeList_.at(i);
        if(id <= value) return value;
    }
    return this->nodeList_.at(0);
}

void ChordNode::setStatusString(const char *s)
{
    if (hasGUI ()) {
        getDisplayString ().setTagArg ("t", 0, s);
        bubble (s);
    }
}
//@}




