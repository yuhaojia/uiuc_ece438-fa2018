#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
using namespace std;

//#define INFINITY 999999999
#define UNDEFINED -1
#define BIGUNDEFINED -999

class Edge {
    public:
        int left,right,weight;
        
        Edge(){
            left = UNDEFINED;
            right = UNDEFINED;
            weight = 1;
        }
        
        Edge(int s, int d, int w){
            if(s < d){
                left = s;
                right = d;
            }else{
                left = d;
                right = s;
            }
            weight = w;
        }
        
        Edge(int s, int d){
            if(s < d){
                left = s;
                right = d;
            }else{
                left = d;
                right = s;
            }
            weight = 1;
        }
        
        Edge(const Edge & e){
            left = e.left;
            right = e.right;
            weight = e.weight;
        }
        
        Edge operator= (const Edge& e) const{
            Edge ret(e.left,e.right,e.weight);
            return ret;
        }
};

class Node {
    public:
        int ID;
        vector<Edge> Edges;
        
        Node(){
            ID = UNDEFINED;
            Edges.clear();
        }
        
        Node(int Id){
            ID = Id;
            Edges.clear();
        }
        
        Node(int Id, vector<Edge> edges){
            for(int i = 0; i < edges.size(); i++){
                Edges.push_back(edges[i]);
            }
            ID = Id;
        }
        
        Node(const Node &n){
            ID = n.ID;
            for(int i = 0; i < n.Edges.size(); i++){
                Edges.push_back(n.Edges[i]);
            }
        }
        
        Node operator= (const Node& n) const{
            Node ret;
            ret.ID = n.ID;
            for(int i = 0; i < n.Edges.size();i++){
                ret.Edges.push_back(n.Edges[i]);
            }
            return ret;
        }
        
        int AddEdge(Edge e){
            if (e.left != ID && e.right != ID)
                return -1;
            Edges.push_back(e);
            return 1;
        }

        int UpdateEdge(Edge e){
            for(int i = 0; i<Edges.size(); i++){
                if(Edges[i].left == e.left && Edges[i].right == e.right){
                    if(e.weight <0){
                        vector<Edge> tmp_vec;
                        for(int j = 0 ; j < Edges.size(); j++){
                            if(j != i){
                                tmp_vec.push_back(Edges[j]);
                            }
                        }
                        Edges.clear();
                        for(int j = 0; j < tmp_vec.size(); j++){
                            Edges.push_back(tmp_vec[j]);
                        }
                        //Edges.erase(Edges.begin()+(i));
                       
                        return 1;
                    }else{
                        Edges[i].weight = e.weight;
                        return 1;
                    }
                }
            }
            AddEdge(e);
            return 1;
        }
};




class Graph {
public:
    unordered_map<int, Node> Nodes;
    vector<Edge> Edges;
    int MaxNodeID;
    
    Graph(){
        Nodes.clear();
        Edges.clear();
        MaxNodeID = UNDEFINED;
    }
    
    void UpdateGraph(Edge deltaEdge){
        if(deltaEdge.left == deltaEdge.right && !HasNode(deltaEdge.left)){
            //actually node and doesn't have it.
            Node n(deltaEdge.left);
            Nodes.insert(make_pair(deltaEdge.left,n));
            return;
        }
        
        //Nodes[deltaEdge.left] = Nodes[deltaEdge.left].UpdateEdge(deltaEdge);
        Node tmp1 = Nodes[deltaEdge.left];
        tmp1.UpdateEdge(deltaEdge);
        Nodes.erase(deltaEdge.left);
        Nodes.insert(make_pair(deltaEdge.left,tmp1));
        //Nodes[deltaEdge.right] = Nodes[deltaEdge.right].UpdateEdge(deltaEdge);
        Node tmp2 = Nodes[deltaEdge.right];
        tmp2.UpdateEdge(deltaEdge);
        Nodes.erase(deltaEdge.right);
        Nodes.insert(make_pair(deltaEdge.right,tmp2));
        
        
        for(int i = 0; i < Edges.size(); i++){
            if(Edges[i].left == deltaEdge.left && Edges[i].right == deltaEdge.right && deltaEdge.weight == BIGUNDEFINED){
                vector<Edge> tmp_vec;
                for(int j = 0 ; j < Edges.size(); j++){
                    if(j != i){
                        tmp_vec.push_back(Edges[j]);
                    }
                }
                Edges.clear();
                for(int j = 0; j < tmp_vec.size(); j++){
                    Edges.push_back(tmp_vec[j]);
                }
                //Edges.erase(Edges.begin()+i);
                return;
            }else if (Edges[i].left == deltaEdge.left && Edges[i].right == deltaEdge.right){
                Edges[i].weight = deltaEdge.weight;
                return;
            }
        }
        Edges.push_back(deltaEdge);
        return;
    }
    
    bool HasNode(int i){
        if (Nodes.find(i) != Nodes.end())
            return true;
        return false;
    }
    
    
    void PrintGraph(){
        cout<<"Print Graph:Nodes"<<endl;
        for ( auto it = Nodes.begin(); it != Nodes.end(); ++it ){
            cout << "ID:" << it->first <<endl;
            for(int j = 0; j < it->second.Edges.size();j++){
                cout << "   " << it->second.Edges[j].left <<"-->"<<it->second.Edges[j].right<<":"<<it->second.Edges[j].weight<<endl;
            }
        }
        cout<<"Print Graph:Edges"<<endl;
        for(int i = 0; i < Edges.size(); i++){
            cout << "   " << Edges[i].left <<"-->"<<Edges[i].right<<":"<<Edges[i].weight<<endl;
        }
    }    
};






