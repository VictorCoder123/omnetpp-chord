//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package CS6381_Chord_P2P;

import inet.applications.contract.ITCPApp;

// Logic for the ChordNode. Modified from TCPBasicClientApp and 
// TCPGenericSrvApp available in the INET framework

simple ChordNode like ITCPApp
{
    parameters:
        @display("i=block/app");
        int localPort = default(10000); // port number to listen on
		
    gates:
        // since we are TCP appln, these are the gates we have
        input tcpIn @labels(TCPCommand/up);
        output tcpOut @labels(TCPCommand/down);
}


// The client Node. Modified from TCPBasicClientApp and 
// TCPGenericSrvApp available in the INET framework

simple Client like ITCPApp
{
    parameters:
        @display("i=block/app");
        @signal[sentLookupTS](type=simtime_t);  // signal emitted when lookup request is sent
        @signal[rcvdRespTS](type=simtime_t);    // signal emitted when response is received
        @signal[CS6381_ClientDone](type=bool);  // emitted by the client when it is done

        @statistic[sentLookupTS](record=vector; title="Timestamp when Lookup Request sent");
        @statistic[rcvdRespTS](record=vector; title="Timestamp when Response received");

        string myID = default("client");	// some id
        int chordNodePort = default(10000); // port number of the chord node we do lookup on

    gates:
        // since we are a TCP application, this is all we have
        input tcpIn @labels(TCPCommand/up);
        output tcpOut @labels(TCPCommand/down);
}

// we are going to have a Coordinator module whose job in life is to receive all the
// signals and take actions like stopping the simulation. It also helps to bootstrap
// various things like initializing the helper object and having it build a node list
// given parameters "m" and "numChordNodes"
simple Coordinator
{
    parameters:
	    int m;	// number of entries in finger table (2^m is the total key space)
	    int numChordNodes;   // number of nodes on the ring; supplied as a parameter
	    int numClients;		 // number of clients in the system; supplied as parameter
	    int numLookupKeys;	 // number of key lookups that a client wants to initiate
	    int numItersPerLookup;	// number of iterations of the same lookup request sent by a client
}
