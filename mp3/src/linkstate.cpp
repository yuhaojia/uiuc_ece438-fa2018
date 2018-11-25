#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <fstream>
#include <sstream>
#include "Graph.h"
using namespace std;

unordered_map<int, unordered_map<int, int>> CostTable;
unordered_map<int, unordered_map<int, int>> ForwardTable;
string outfile = "output.txt";
ofstream outfilestream(outfile, ios_base::app);

void MessageProc(string line, int &src, int &dst, string &msg){
    size_t pos0, pos1, pos2;
    pos0 = line.find_first_of(" ");
    pos1 = line.find_first_of(" ", pos0+1);
    pos2 = line.length();
    src = stoi(line.substr(0,pos0 - 0));
    dst = stoi(line.substr(pos0+1,pos1 - pos0 - 1));
    msg = line.substr(pos1+1,pos2 - pos1 - 1);
}

int TieBreak(unordered_map<int,Node>& Q,unordered_map<int,int>& dist){
    int ret = -1;
    int MinDist = BIGINTNUM;
    for ( auto it = dist.begin(); it != dist.end(); ++it ){
        if(it->second < MinDist && Q.find(it->first) != Q.end()){
            ret = it->first;
            MinDist = it->second;
        }else if (it->second == MinDist && Q.find(it->first) != Q.end()){  //Tie Break 2 or 3?;
            if (ret > it->first){
                ret = it->first;
            }
        }
    }
    return ret;
}

void Dijkstra(Graph g, int src, unordered_map<int,int> & dist, unordered_map<int,int> & prev){
    if(dist.size() != 0 || prev.size() != 0){
        return;
    }
    unordered_map<int,Node> Q;
    //Initialization
    for ( auto it = g.Nodes.begin(); it != g.Nodes.end(); ++it ){ //
        dist[it->first] = BIGINTNUM;
        prev[it->first] = UNDEFINED;
        Q.insert(make_pair(it->first,it->second));
    }
    
    dist[src] = 0;
    while(Q.size()!= 0){
        int uID = TieBreak(Q,dist);
        if(uID == -1){
            break;
        }
        Node u = Q[uID];
        Q.erase(uID);
        for(int i = 0; i < u.Edges.size(); i++){
            Edge e = u.Edges[i];
            int vID;
            if (e.left == uID){
                vID = e.right;
            }
            else {
                vID = e.left;
            }
            if (Q.find(vID) == Q.end())
                continue;
            int alt = dist[uID] + e.weight;
            if (alt < dist[vID] && uID != UNDEFINED && vID != UNDEFINED){
                dist[vID] = alt;
                prev[vID] = uID;
            }else if (alt == dist[vID] && uID < prev[vID] && uID != UNDEFINED && vID != UNDEFINED){
                prev[vID] = uID;
            }
        }
    }
    return;
}

void OutTopology(Graph g, int src,unordered_map<int,int> & dist, unordered_map<int,int> & prev){
    //ofstream outfile;
    //outfile.open("output.txt", ios_base::app);
    //outfile<<endl;
    //cout<<endl;
    vector<int> stack;
    int dst = 0;
    while(dst <= g.MaxNodeID){
        bool flagdst = false;
        if (g.Nodes.find(dst) != g.Nodes.end())
            flagdst = true;
        if(!flagdst){
            dst++;
            continue;
        }
        if(dist[dst] == BIGINTNUM){
            dst++;
            continue;
        }
        stack.clear();
        int tmp = dst;
        stack.push_back(dst);
        while(tmp != src){
            stack.push_back(prev[tmp]);
            tmp = prev[tmp];
        }
        if(stack.size()==1){
            outfilestream<<dst<<" "<<stack[0]<<" "<<dist[dst]<<endl;
            cout<<dst<<" "<<stack[0]<<" "<<dist[dst]<<endl;
        }else{
            outfilestream<<dst<<" "<<stack[stack.size()-2]<<" "<<dist[dst]<<endl;
            cout<<dst<<" "<<stack[stack.size()-2]<<" "<<dist[dst]<<endl;
        }
        dst++;
    }
    outfilestream<<endl;
    cout<<endl;
    //outfile.close();
}
    

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }
    string topofile(argv[1]);
    string messagefile(argv[2]);
    string changesfile(argv[3]);
    //string outfile = "output.txt";
    
    Graph g;
    ifstream infile(topofile);
    int ss, dd, ww;
    while (infile >> ss >> dd >> ww)
    {   
        Edge e(ss,dd,ww);
        bool flags = false;
        bool flagd = false;
        if (g.Nodes.find(ss) != g.Nodes.end())
            flags = true;
        if (g.Nodes.find(dd) != g.Nodes.end())
            flagd = true;
        if(ss == dd && flags){
            continue;
        }else if (ss==dd && !flags){
            g.Nodes.insert(make_pair(ss,Node(ss)));
            continue;
        }
        g.Edges.push_back(e);
        if(!flags){
            Node n(ss);
            n.AddEdge(e);
            g.Nodes.insert(make_pair(ss,n));
        }else{
            g.Nodes[ss].AddEdge(e);
        }
        
        if(!flagd){
            Node n(dd);
            n.AddEdge(e);
            g.Nodes.insert(make_pair(dd,n));
        }else{
            g.Nodes[dd].AddEdge(e);
        }
    }
    g.MaxNodeID = UNDEFINED;
    for(auto it = g.Nodes.begin(); it != g.Nodes.end(); ++it){
        if(it->first >g.MaxNodeID)
            g.MaxNodeID = it->first;
    }


    //solver.LinkState(g);
    CostTable.clear();
    ForwardTable.clear();
    for(int i = 0; i <= g.MaxNodeID; i++){
        bool flagi = false;
        if (g.Nodes.find(i) != g.Nodes.end())
            flagi = true;
        if(!flagi)
            continue;
        int src = i;
        unordered_map<int,int> dist,prev;
        Dijkstra(g,src,dist,prev);
        OutTopology(g, src, dist, prev);
        CostTable.insert(make_pair(src, dist));
        ForwardTable.insert(make_pair(src,prev));
    }

    //solver.OutputMessage(outfile, messagefile);
    //ofstream outfilestream;
    //outfilestream.open(outfile, ios_base::app);
    ifstream mfile(messagefile);
    string line;
    while(getline(mfile,line)){
        int src,dst;
        string msg = "";
        MessageProc(line, src, dst, msg);

        if(CostTable[src][dst] == BIGINTNUM){
            outfilestream<<"from "<<src<<" to "<<dst<<" cost infinite hops unreachable message "<<msg<<endl;
            cout<<"from "<<src<<" to "<<dst<<" cost infinite hops unreachable message "<<msg<<endl;
            continue;
        }else{
            outfilestream<<"from "<<src<<" to "<<dst<<" cost "<<CostTable[src][dst]<<" hops ";
            cout<<"from "<<src<<" to "<<dst<<" cost "<<CostTable[src][dst]<<" hops ";
        }
        vector<int> stack;
        int tmp = dst;
        while(tmp != src){
            stack.push_back(ForwardTable[src][tmp]);
            tmp = ForwardTable[src][tmp];
        }
        for(int i = int(stack.size()-1); i>= 0; i--){
            outfilestream<<stack[i]<<" ";
            cout<<stack[i]<<" ";
        }
        outfilestream<<"message "<<msg<<endl;
        cout<<"message "<<msg<<endl;
    }
    //outfilestream.close();

    ifstream cfile(changesfile);
    int s,d,w;
    while(cfile>>s>>d>>w){
        g.UpdateGraph(Edge(s,d,w));

        //solver.LinkState(g);
        CostTable.clear();
        ForwardTable.clear();
        for(int i = 0; i <= g.MaxNodeID; i++){
            bool flagi = false;
            if (g.Nodes.find(i) != g.Nodes.end())
                flagi = true;
            if(!flagi)
                continue;
            int src = i;
            unordered_map<int,int> dist,prev;
            Dijkstra(g,src,dist,prev);
            OutTopology(g, src, dist, prev);

            CostTable.insert(make_pair(src, dist));
            ForwardTable.insert(make_pair(src,prev));
        }

        //solver.OutputMessage(outfile, messagefile);
        //ofstream outfilestream;
        //outfilestream.open(outfile, ios_base::app);
        //ifstream mfile(messagefile);
        string line;
        while(getline(mfile,line)){
            int src,dst;
            string msg = "";
            MessageProc(line, src, dst, msg);
            if(CostTable[src][dst] == BIGINTNUM){
                outfilestream<<"from "<<src<<" to "<<dst<<" cost infinite hops unreachable message "<<msg<<endl;
                cout<<"from "<<src<<" to "<<dst<<" cost infinite hops unreachable message "<<msg<<endl;
                continue;
            }else{
                outfilestream<<"from "<<src<<" to "<<dst<<" cost "<<CostTable[src][dst]<<" hops ";
                cout<<"from "<<src<<" to "<<dst<<" cost "<<CostTable[src][dst]<<" hops ";
            }
            vector<int> stack;
            int tmp = dst;
            while(tmp != src){
                stack.push_back(ForwardTable[src][tmp]);
                tmp = ForwardTable[src][tmp];
            }
            for(int i = int(stack.size()-1); i>= 0; i--){
                outfilestream<<stack[i]<<" ";
                cout<<stack[i]<<" ";
            }
            outfilestream<<"message "<<msg<<endl;
            cout<<"message "<<msg<<endl;
        }
        //outfilestream.close();


    }
    outfilestream.close();
    return 0;
}