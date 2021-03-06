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

int TieBreak(int src, int dst, unordered_map<int,int> & prev){
    int temp = dst;
    int result = UNDEFINED;
    while(temp != src){
        if(temp == UNDEFINED)
            return temp;
        result = temp;
        temp = prev[temp];
    }
    return result;
}

void BellmanFord(Graph g, int src, unordered_map<int,int> & dist, unordered_map<int,int> & prev){
    if(dist.size() != 0 || prev.size() != 0){
        return;
    }
    
    for ( auto it = g.Nodes.begin(); it != g.Nodes.end(); ++it ){ //
        dist[it->first] = BIGINTNUM;
        prev[it->first] = UNDEFINED;
    }
    
    dist[src] = 0;
    
    for (int i = 1; i < g.Nodes.size(); i++){
        for(int j = 0; j < g.Edges.size(); j++){
            int u = g.Edges[j].left;
            int v = g.Edges[j].right;
            int w = g.Edges[j].weight;
            if(dist[u] + w < dist[v]){
                dist[v] = dist[u] + w;
                prev[v] = u;
            }else if (dist[u]+w == dist[v]){ //tie break1
                int t1_u = TieBreak(src, u, prev);
                int t1_v = TieBreak(src, v, prev);
                if(t1_u != UNDEFINED && t1_v != UNDEFINED && t1_u < t1_v)
                    prev[v] = u;
            }
            if(dist[v] + w < dist[u]){
                dist[u] = dist[v] + w;
                prev[u] = v;
            }else if (dist[v] + w == dist[u] && v < prev[u]){
                int t1_u = TieBreak(src, u, prev);
                int t1_v = TieBreak(src, v, prev);
                if(t1_u != UNDEFINED && t1_v != UNDEFINED && t1_v < t1_u)
                    prev[u] = v;
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
    //printf("Number of arguments: %d", argc);
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
        BellmanFord(g,src,dist,prev);
        OutTopology(g, src, dist, prev);

        CostTable.insert(make_pair(src, dist));
        ForwardTable.insert(make_pair(src,prev));
    }

    ofstream outfilestream;
    outfilestream.open(outfile, ios_base::app);
    ifstream mfile(messagefile);
    string line;
    while(getline(mfile,line)){
        //TODO:
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
            BellmanFord(g,src,dist,prev);
            OutTopology(g, src, dist, prev);

            CostTable.insert(make_pair(src, dist));
            ForwardTable.insert(make_pair(src,prev));
        }

        ofstream outfilestream;
        outfilestream.open(outfile, ios_base::app);
        ifstream mfile(messagefile);
        string line;
        while(getline(mfile,line)){
            //TODO:
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
        outfilestream.close();


    }
    
    return 0;
}