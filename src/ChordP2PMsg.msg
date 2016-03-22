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

// packet formats for the request and response of the lookup method used by clients
// when they lookup a key on a DHT node
packet Lookup_Req
{
    long	key;		// lookup key
    string	sender;		// sender
};

packet Lookup_Resp
{
	long	key;		// lookup key
	string	sender;		// id of the sender
	string	responder [];	// list of chord nodes 
};
