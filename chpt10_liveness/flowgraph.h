#ifndef TIGER_COMPILER_FLOWGRAPH_H
#define TIGER_COMPILER_FLOWGRAPH_H

/*
 * flowgraph.h - Function prototypes to represent control flow graphs.
 */

Temp_tempList FG_def(G_node n);
Temp_tempList FG_use(G_node n);
bool FG_isMove(G_node n);
G_graph FG_AssemFlowGraph(AS_instrList il);


void FG_Showinfo(FILE *out, AS_instr instr, Temp_map map);

#endif
