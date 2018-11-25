#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
using namespace std;

//#define INFINITY 999999999
#define UNDEFINED -1
#define BIGUNDEFINED -999
#define BIGINTNUM 999999999

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

        void EraseNode(int i){
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
        }

        int UpdateEdge(Edge e){
            for(int i = 0; i<Edges.size(); i++){
                if(Edges[i].left == e.left && Edges[i].right == e.right){
                    if(e.weight <0){
                        EraseNode(i);
                        //Edges.erase(Edges.begin() + i);
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

        void EraseNode(int i){
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
        }
        
        void UpdateGraph(Edge EdgetoUpdate){
            bool flagNode = false;
            if (Nodes.find(EdgetoUpdate.left) != Nodes.end())
                flagNode = true;
            if(EdgetoUpdate.left == EdgetoUpdate.right && !flagNode){
                Node n(EdgetoUpdate.left);
                Nodes.insert(make_pair(EdgetoUpdate.left,n));
                return;
            }
            
            Node tmp1 = Nodes[EdgetoUpdate.left];
            Node tmp2 = Nodes[EdgetoUpdate.right];
            tmp1.UpdateEdge(EdgetoUpdate);
            tmp2.UpdateEdge(EdgetoUpdate);
            Nodes.erase(EdgetoUpdate.left);
            Nodes.erase(EdgetoUpdate.right);
            Nodes.insert(make_pair(EdgetoUpdate.left,tmp1));
            Nodes.insert(make_pair(EdgetoUpdate.right,tmp2));

            for(int i = 0; i < Edges.size(); i++){
                if(Edges[i].left == EdgetoUpdate.left && Edges[i].right == EdgetoUpdate.right && EdgetoUpdate.weight == BIGUNDEFINED){
                    EraseNode(i);
                    //Edges.erase(Edges.begin()+i);
                    return;
                }else if (Edges[i].left == EdgetoUpdate.left && Edges[i].right == EdgetoUpdate.right){
                    Edges[i].weight = EdgetoUpdate.weight;
                    return;
                }
            }
            Edges.push_back(EdgetoUpdate);
            return;
        }
};






