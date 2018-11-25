#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <fstream>
#include <sstream>
#include "Graph.h"
using namespace std;

unordered_map<int, unordered_map<int, int>> CostTable;
unordered_map<int, unordered_map<int, int>> ForwardTable;
//string outfile = "output.txt";
//ofstream outfilestream(outfile, ios_base::app);

void MessageProc(string line, int &src, int &dst, string &msg){
    size_t mes1, mes2, mes3;
    mes1 = line.find_first_of(" ");
    mes2 = line.find_first_of(" ", mes1+1);
    mes3 = line.length();
    src = stoi(line.substr(0,mes1 - 0));
    dst = stoi(line.substr(mes1+1,mes2 - mes1 - 1));
    msg = line.substr(mes2+1,mes3 - mes2 - 1);
}

int TieBreak(unordered_map<int,Node>& Q,unordered_map<int,int>& dist){
    int result = -1;
    int MinDist = 999999999;
    for ( auto it = dist.begin(); it != dist.end(); ++it ){
        if(it->second < MinDist && Q.find(it->first) != Q.end()){
            result = it->first;
            MinDist = it->second;
        }else if (it->second == MinDist && Q.find(it->first) != Q.end()){
            if (result > it->first){
                result = it->first;
            }
        }
    }
    return result;
}

void Dijkstra(Graph g, int src, unordered_map<int,int> & dist, unordered_map<int,int> & prev){
    if(dist.size() != 0 || prev.size() != 0){
        return;
    }
    unordered_map<int,Node> Q;
    //Initialization
    for ( auto it = g.Nodes.begin(); it != g.Nodes.end(); ++it ){ //
        dist[it->first] = 999999999;
        prev[it->first] = UNDEFINED;
        Q.insert(make_pair(it->first,it->second));
    }
    
    dist[src] = 0;
    while(Q.size()!= 0){
        int IDTB = TieBreak(Q,dist);
        if(IDTB == -1){
            break;
        }
        Node u = Q[IDTB];
        Q.erase(IDTB);
        for(int i = 0; i < u.Edges.size(); i++){
            Edge e = u.Edges[i];
            int vID;
            if (e.left == IDTB){
                vID = e.right;
            }
            else {
                vID = e.left;
            }
            if (Q.find(vID) == Q.end())
                continue;
            int alt = dist[IDTB] + e.weight;
            if (alt < dist[vID] && IDTB != UNDEFINED && vID != UNDEFINED){
                dist[vID] = alt;
                prev[vID] = IDTB;
            }else if (alt == dist[vID] && IDTB < prev[vID] && IDTB != UNDEFINED && vID != UNDEFINED){
                prev[vID] = IDTB;
            }
        }
    }
    return;
}

void OutTopology(Graph g, int src,unordered_map<int,int> & dist, unordered_map<int,int> & prev){
    ofstream outfile;
    outfile.open("output.txt", ios_base::app);
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
        if(dist[dst] == 999999999){
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
            outfile<<dst<<" "<<stack[0]<<" "<<dist[dst]<<endl;
            cout<<dst<<" "<<stack[0]<<" "<<dist[dst]<<endl;
        }else{
            outfile<<dst<<" "<<stack[stack.size()-2]<<" "<<dist[dst]<<endl;
            cout<<dst<<" "<<stack[stack.size()-2]<<" "<<dist[dst]<<endl;
        }
        dst++;
    }
    outfile<<endl;
    cout<<endl;
    outfile.close();
}
    

int main(int argc, char** argv) {
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }
    string topofile(argv[1]);
    string messagefile(argv[2]);
    string changesfile(argv[3]);
    string outfile = "output.txt";
    
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

    ofstream outfilestream;
    outfilestream.open(outfile, ios_base::app);
    ifstream mfile(messagefile);
    string line;
    while(getline(mfile,line)){
        int src,dst;
        string msg = "";
        MessageProc(line, src, dst, msg);

        if(CostTable[src][dst] == 999999999){
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
    outfilestream.close();

    ifstream cfile(changesfile);
    int s,d,w;
    while(cfile>>s>>d>>w){
        g.UpdateGraph(Edge(s,d,w));

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

        ofstream outfilestream;
        outfilestream.open(outfile, ios_base::app);
        ifstream mfile(messagefile);

        string line;
        while(getline(mfile,line)){
            int src,dst;
            string msg = "";
            MessageProc(line, src, dst, msg);
            if(CostTable[src][dst] == 999999999){
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
        outfilestream.close();

    }
    
    return 0;
}