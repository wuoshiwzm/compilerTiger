#ifndef TIGER_COMPILER_FLOWGRAPH_H
#define TIGER_COMPILER_FLOWGRAPH_H

/*
 * flowgraph.h - Function prototypes to represent control flow graphs.
 */
// 节点 n 的定值 def
Temp_tempList FG_def(G_node n);

// 节点 n 的使用 use
Temp_tempList FG_use(G_node n);

// 节点 n 的后继
G_nodeList FG_succ(G_node n);

// 节点 n 是否为 MOVE 指令
bool FG_isMove(G_node n);

// 将指令表 AS_instrList 转换为图 G_graph
G_graph FG_AssemFlowGraph(AS_instrList il);

void FG_Showinfo(FILE *out, AS_instr instr, Temp_map map);


#endif
