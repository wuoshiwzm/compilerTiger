#ifndef _GRAPH_H_
#define _GRAPH_H_


// typedef struct G_nodeList_ *G_nodeList;

// struct G_nodeList_{
//     G_node head; 
//     G_nodeList tail; 
// };

// G_graph G_Graph(void);
// G_node G_Node(G_graph g, void *info);
// G_nodeList G_NodeList(G_node head, G_nodeList tail);
// G_nodeList G_nodes(G_graph g); // 获得图中的所有节点

// bool G_inNodeList(G_node node, G_nodeList list);

// void G_addEdge(G_node from, G_node to); // 创建一条新的边
// void G_rmEdge(G_node from, G_node to); // 删除一条边
// void G_show(FILE *out, G_nodeList p, void showInfo(void *)); // 

// G_nodeList G_succ(G_node n); // 获得节点 n 的所有后继
// G_nodeList G_pred(G_node n); // 获得节点 n 的所有前置
// G_nodeList G_adj(G_node n); // G_succ(n) U G_pred(n)

// bool G_goesTo(G_node a, G_node b); // a,b 之前是否有边
// int G_degree(G_node n); // 节点的度 入边+出边
// void *G_nodeInfo(G_node n); // 节点信息

// typedef struct TAB_table_ *G_table;
// G_table G_empty(void); // 创建新表
// void G_enter(G_table t, G_node n, void *v); // 在表 t 中建立 n->v
// void *G_look(G_table t, G_node n); // 查找 n->v, 并报告v



/*
 * graph.h - Abstract Data Type (ADT) for directed graphs
 */

typedef struct G_graph_ *G_graph;  /* The "graph" type */
typedef struct G_node_ *G_node;    /* The "node" type */

typedef struct G_nodeList_ *G_nodeList;
struct G_nodeList_ { G_node head; G_nodeList tail;};

/* Make a new graph */
G_graph G_Graph(void); 
/* Make a new node in graph "g", with associated "info" */
G_node G_Node(G_graph g, void *info);

/* Make a NodeList cell */
G_nodeList G_NodeList(G_node head, G_nodeList tail);

/* Get the list of nodes belonging to "g" */
G_nodeList G_nodes(G_graph g);

/* Tell if "a" is in the list "l" */
bool G_inNodeList(G_node a, G_nodeList l);

/* Make a new edge joining nodes "from" and "to", which must belong
    to the same graph */
void G_addEdge(G_node from, G_node to);

/* Delete the edge joining "from" and "to" */
void G_rmEdge(G_node from, G_node to);

/* Show all the nodes and edges in the graph, using the function "showInfo"
    to print the name of each node */
void G_show(FILE *out, G_nodeList p, void showInfo(void *));

/* Get all the successors of node "n" */
G_nodeList G_succ(G_node n);

/* Get all the predecessors of node "n" */
G_nodeList G_pred(G_node n);

/* Tell if there is an edge from "from" to "to" */
bool G_goesTo(G_node from, G_node n);

/* Tell how many edges lead to or from "n" */
int G_degree(G_node n);

/* Get all the successors and predecessors of "n" */
G_nodeList G_adj(G_node n);

/* Get the "info" associated with node "n" */
void *G_nodeInfo(G_node n);

/* The type of "tables" mapping graph-nodes to information */
typedef struct TAB_table_  *G_table;

/* Make a new table */
G_table G_empty(void);

/* Enter the mapping "node"->"value" to the table "t" */
void G_enter(G_table t, G_node node, void *value);

/* Tell what "node" maps to in table "t" */
void *G_look(G_table t, G_node node);


#endif