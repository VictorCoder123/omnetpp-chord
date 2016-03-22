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

package CS6381_Chord_P2P.simulations;

// the TCP appln level functionality 
import inet.applications.tcpapp.TCPBasicClientApp;
import inet.applications.tcpapp.TCPGenericSrvApp;

// import our ned defn for the appln logic
import CS6381_Chord_P2P.Client;
import CS6381_Chord_P2P.ChordNode;
import CS6381_Chord_P2P.Coordinator;


// defns of different host types
import CS6381_Chord_P2P.simulations.ClientHost;
import CS6381_Chord_P2P.simulations.ChordHost;

// import the channel defns
import CS6381_Chord_P2P.simulations.EthernetCable;
import CS6381_Chord_P2P.simulations.FiberLine;

// ethernet hub
import inet.linklayer.ethernet.EtherHub;

// ethernet switch
import inet.node.ethernet.EtherSwitch;

// router defn
import inet.node.inet.Router;

// automatic configuration of IPv4 addresses
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;

// some common defns
network CS6381_Chord_Base
{
    parameters:
        // indicates how many bits
        int m = default(4);

        // indicates how many chord nodes
        int numChordNodes = default(5);

        // indicates how many client nodes
        int numClients = default(1);

        // indicates number of lookup keys
        int numLookupKeys = default(10);

        // indicates number of iterations per lookup
        int numItersPerLookup = default(2);

    submodules:

        coordinator: Coordinator {
            m = m;
            numChordNodes = numChordNodes;
            numClients = numClients;
            numLookupKeys = numLookupKeys;
            numItersPerLookup = numItersPerLookup;
        }
}

// a basic wired LAN with an ethernet hub
network CS6381_Chord_LAN_wHub extends CS6381_Chord_Base
{
    submodules:
        // this one configures the IPv4 network
        configurator: IPv4NetworkConfigurator {
            parameters:
                config = xml("<config><interface hosts='*' address='192.168.x.x' netmask='255.255.x.x'/></config>");
        }

        // the ethernet hub
        hub: EtherHub;
        // the clients
        client[numClients]: ClientHost;
        // all the hosts running the chord logic
        chordHosts[numChordNodes]: ChordHost;
    connections:
        // connect all our clients to the ethernet hub via the ethernet cable
        for i=0..numClients-1 {
            client[i].ethg++ <--> EthernetCable <--> hub.ethg++;
        }

        for i=0..numChordNodes-1 {
            chordHosts[i].ethg++ <--> EthernetCable <--> hub.ethg++;
        }
}

// a basic wired LAN with an ethernet switch
network CS6381_Chord_LAN_wSwitch extends CS6381_Chord_Base
{
    submodules:
        // this one configures the IPv4 network
        configurator: IPv4NetworkConfigurator {
            parameters:
                config = xml("<config><interface hosts='*' address='192.168.x.x' netmask='255.255.x.x'/></config>");
        }

        // the ethernet hub
        switch: EtherSwitch {

            @display("p=273,228");
        }
        // the clients
        client[numClients]: ClientHost;
        // all the hosts running the chord logic
        chordHosts[numChordNodes]: ChordHost;
    connections:
        // connect all our clients to the ethernet hub via the ethernet cable
        for i=0..numClients-1 {
            client[i].ethg++ <--> EthernetCable <--> switch.ethg++;
        }

        for i=0..numChordNodes-1 {
            chordHosts[i].ethg++ <--> EthernetCable <--> switch.ethg++;
        }
}
