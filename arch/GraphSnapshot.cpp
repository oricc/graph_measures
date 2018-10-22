#include "stdafx.h"
#include "GraphSnapshot.h"
#include <iomanip>
#include <exception>
//#include <boost/graph/adjacency_list.hpp>
#pragma warning(push)
#pragma warning(disable: 4996) //4996 for _CRT_SECURE_NO_WARNINGS equivalent
#include <numeric>
#include <random>
#pragma warning(pop)

//#include <set>

/*
	Clear up all memory allocations and internal variables.
*/
void GraphSnapshot::Clear()
{
	m_NumberOfNodes = 0; 
	delete m_Graph; m_Graph = NULL; 
	delete m_Offsets; m_Offsets = NULL; 
}

/*
	Initialize the graph from two lists (the offset list and the adjacency list).
*/
void GraphSnapshot::Assign(const std::vector<unsigned __int64>& NodeOffsets, const std::vector<unsigned int>& Neighbours)
{
	//Clear the graph so we can assign new data
	Clear(); 

	//copy the offset list into a new array
	m_NumberOfNodes = static_cast<unsigned int>(NodeOffsets.size()-1);
	m_Offsets = new unsigned __int64[m_NumberOfNodes + 1];
	std::memcpy(m_Offsets, &NodeOffsets[0], NodeOffsets.size() * sizeof(unsigned __int64));
	
	//copy the adjacency list into a new array
	m_Graph = new unsigned int[m_Offsets[m_NumberOfNodes]];
	std::memcpy(m_Graph, &Neighbours[0], Neighbours.size() * sizeof(unsigned int));
	/*LOG(INFO) << m_NumberOfNodes << '\t' << m_Offsets[m_NumberOfNodes];*/
	std::cout << m_NumberOfNodes << '\t' << m_Offsets[m_NumberOfNodes];
}

/*
	Save the graph to a binary file.
*/
bool GraphSnapshot::SaveToFile(const std::string& FileName) const
{
	//open or create a file of a binary format
	FILE* hFile = std::fopen(FileName.c_str(), "w+b");
	
	//write the class variables to the file
	std::fwrite(&m_NumberOfNodes, sizeof(unsigned int), 1, hFile);
	std::fwrite(&m_Offsets[m_NumberOfNodes], sizeof(unsigned __int64), 1, hFile);
	std::fwrite(m_Offsets, sizeof(unsigned __int64), m_NumberOfNodes+1, hFile);
	std::fwrite(m_Graph, sizeof(unsigned int), m_Offsets[m_NumberOfNodes], hFile);
	
	//close the file
	std::fclose(hFile); hFile = NULL;

	//LOG(INFO) << m_NumberOfNodes << '\t' << m_Offsets[m_NumberOfNodes];
	std::cout << m_NumberOfNodes << '\t' << m_Offsets[m_NumberOfNodes];

	return true; 
}

/*
	Utility function: append the directory and file names into one string
*/
std::string GraphSnapshot::GetFileNameFromFolder(const std::string& DirectroyName, const std::string& BaseFileName)
{
	std::stringstream FileName;
	FileName << DirectroyName << BaseFileName << "_" << std::setfill('0') << std::setw(2) << ".bin";
	return FileName.str();
}

/*
	Utility function: create the full path from the directory and the file name and then call the 
	overloaded function.
*/
bool GraphSnapshot::LoadFromFile(const std::string& DirectroyName, const std::string& BaseFileName)
{
	std::string FileName = GetFileNameFromFolder(DirectroyName, BaseFileName);
	return LoadFromFile(FileName);
}

/*
	Read the data from the binary file
*/
bool GraphSnapshot::LoadFromFile(const std::string& FileName)
{
	Clear(); 
	FILE* hFile;
	//if (!Utilities::FileExists(FileName)) { LOG(ERROR) << " Missing network file. Run GraphSnapshot::LoadFromFile - " << FileName; return false; }
	//open network file
	try {
		hFile = std::fopen(FileName.c_str(), "rb");
	}
	catch(std::exception e) {
		/*LOG(ERROR) << " Missing network file. Run GraphSnapshot::LoadFromFile - " << FileName; return false;*/
		std::cout << " Missing network file. Run GraphSnapshot::LoadFromFile - " << FileName; return false;
	}

	//read the number of nodes
	std::fread(&m_NumberOfNodes,sizeof(unsigned int),1, hFile);

	//read the number of edges
	unsigned __int64 NumberOfEdges = 0; 
	std::fread(&NumberOfEdges, sizeof(unsigned __int64), 1, hFile);
	
	//create an array to store the indices (offsets) of the nodes in the graph array and read into it
	m_Offsets = new unsigned __int64[m_NumberOfNodes + 1];
	std::fread(m_Offsets, sizeof(unsigned __int64), m_NumberOfNodes + 1, hFile);
	
	//create the main array containing the lists of neighbors.
	/*
	NOTE:
	Here there is an assumption of a directed graph.
	An undirected graph is created by having two edges saved in the file for each edge
	in the network.
	*/
	m_Graph = new unsigned int[NumberOfEdges];
	std::fread(m_Graph, sizeof(unsigned int), NumberOfEdges, hFile);
		
	std::fclose(hFile); hFile = NULL; 
	return true; 
}

/*
	Assuming our graph is directed, generate the inverted graph.
	i.e. change every edge e=(a,b) to (b,a) in the inverted graph.
*/
void GraphSnapshot::InverseGraph(GraphSnapshot& InvertedGraph) const
{
	//get the number of edges in the graph
	const unsigned __int64 NumberOfEdges = m_Offsets[m_NumberOfNodes];
	//clear the inverted graph
	InvertedGraph.Clear();
	//assign the member variables to the inverted graph
	InvertedGraph.m_NumberOfNodes = m_NumberOfNodes;
	//allocate the needed memory
	InvertedGraph.m_Offsets = new unsigned __int64[m_NumberOfNodes + 1]; 
	InvertedGraph.m_Graph = new unsigned int[NumberOfEdges];
	
	//invert the adjancency list
	unsigned int* InDegrees = new unsigned int[m_NumberOfNodes];
	std::memset(InDegrees, 0, m_NumberOfNodes * sizeof(unsigned int));
	for (const unsigned int* p = m_Graph; p<m_Graph + NumberOfEdges; ++p)
	{
		++InDegrees[*p];
	}
	InvertedGraph.m_Offsets[0] = 0;
	std::partial_sum(InDegrees, InDegrees + m_NumberOfNodes, InvertedGraph.m_Offsets + 1);
	for (unsigned int NodeID = 0; NodeID < m_NumberOfNodes; ++NodeID) {
		for (const unsigned int* peer = m_Graph + m_Offsets[NodeID]; peer < m_Graph+m_Offsets[NodeID+1]; ++peer)
		{
			InvertedGraph.m_Graph[InvertedGraph.m_Offsets[*peer]] = NodeID;
			++InvertedGraph.m_Offsets[*peer];
		}
	}
	InvertedGraph.m_Offsets[0] = 0;
	std::partial_sum(InDegrees, InDegrees + m_NumberOfNodes, InvertedGraph.m_Offsets + 1);
	//clear memory
	delete [] InDegrees; InDegrees = NULL; 
}
void GraphSnapshot::CureateUndirectedGraph(const GraphSnapshot& InvertedGraph, GraphSnapshot& UndirectedGraph) const
{
	//calculate the number of edges in the graph
	const unsigned __int64 NumberOfEdges = m_Offsets[m_NumberOfNodes];
	//clear the new graph
	UndirectedGraph.Clear();
	//assign member variables
	UndirectedGraph.m_NumberOfNodes = m_NumberOfNodes;
	//allocate memory
	UndirectedGraph.m_Offsets = new unsigned __int64[m_NumberOfNodes + 1];
	std::memset(UndirectedGraph.m_Offsets, 0, (m_NumberOfNodes) * sizeof(unsigned __int64));
	//note that in an undirected graph, there are twice as many edges in the
	//adjacency list as in a directed graph.
	unsigned int* temp_Graph = new unsigned int[2 * NumberOfEdges];
	std::memset(temp_Graph, 0, 2 * NumberOfEdges * sizeof(unsigned int));
	unsigned int *temp_graph_pointer = temp_Graph;

	//combine the inverted and normal graphs into an undirected one.
	for (unsigned int NodeID = 0; NodeID < m_NumberOfNodes; ++NodeID)
	{
		auto p1 = m_Graph + m_Offsets[NodeID];
		auto p2 = InvertedGraph.m_Graph + InvertedGraph.m_Offsets[NodeID];
		while (p1 < m_Graph + m_Offsets[NodeID + 1] && p2 < InvertedGraph.m_Graph + InvertedGraph.m_Offsets[NodeID + 1])
		{
			if (*p1 == *p2)
			{
				*temp_graph_pointer = *p1;
				++p1; ++p2;
			}
			else if (*p1 < *p2)
			{
				*temp_graph_pointer = *p1;
				++p1;
			}
			else // if (*p1>*p2)
			{
				*temp_graph_pointer = *p2;
				++p2;
			}
			++temp_graph_pointer;
		}
		if (p1 < m_Graph + m_Offsets[NodeID + 1])
		{
			unsigned __int64 RemainingElements = m_Graph + m_Offsets[NodeID + 1] - p1;
			std::memcpy(temp_graph_pointer, p1, sizeof(unsigned int)*RemainingElements);
			temp_graph_pointer += RemainingElements;
		}
		else if (p2 < InvertedGraph.m_Graph + InvertedGraph.m_Offsets[NodeID + 1])
		{
			unsigned __int64 RemainingElements = InvertedGraph.m_Graph + InvertedGraph.m_Offsets[NodeID + 1] - p2;
			std::memcpy(temp_graph_pointer, p2, sizeof(unsigned int)*RemainingElements);
			temp_graph_pointer += RemainingElements;
		}
		UndirectedGraph.m_Offsets[NodeID + 1] = temp_graph_pointer - temp_Graph;
	}
	UndirectedGraph.m_Graph = new unsigned int[UndirectedGraph.m_Offsets[m_NumberOfNodes]];
	std::memcpy(UndirectedGraph.m_Graph, temp_Graph, UndirectedGraph.m_Offsets[m_NumberOfNodes] * sizeof(unsigned int));
	//clear memory
	delete[] temp_Graph;
}


std::vector<unsigned int> GraphSnapshot::ComputeNodeDegrees() const
{
	std::vector<unsigned int> Degrees(m_NumberOfNodes, 0);
	for (unsigned int NodeID = 0; NodeID < m_NumberOfNodes; ++NodeID)
	{
		Degrees[NodeID] =static_cast<unsigned int>(m_Offsets[NodeID + 1] - m_Offsets[NodeID]);
	}
	return Degrees; 
}

std::vector<float> GraphSnapshot::ComputeNodePageRank(float dumping, unsigned int NumberOfIterations) const
{
	std::random_device rng;
	std::mt19937 urng(rng());
	std::vector<float> PageRank(m_NumberOfNodes, 1.0F-dumping);
	while (NumberOfIterations>0)
	{
		std::vector<int> NodeIDs(m_NumberOfNodes); // vector with 100 ints.
		std::iota(std::begin(NodeIDs), std::end(NodeIDs), 0);
		std::shuffle(NodeIDs.begin(), NodeIDs.end(), urng);
		for (unsigned int index = 0; index < m_NumberOfNodes; ++index)
		{
			const unsigned int NodeID = NodeIDs[index];
			float friends_contribution = 0.0; 			
			for (auto p = m_Graph + m_Offsets[NodeID]; p < m_Graph + m_Offsets[NodeID+1]; ++p)
			{
				friends_contribution += PageRank[*p]/(m_Offsets[*p+1]- m_Offsets[*p]) ;
			}
			PageRank[NodeID] = (1.0F - dumping) + dumping*friends_contribution;
		}
		--NumberOfIterations;
	}
	return PageRank; 
}
std::vector<unsigned short> GraphSnapshot::ComputeKCore() const
{
	const unsigned short UNSET_K_CORE = static_cast<unsigned short>(-1);
	std::vector<unsigned short>  KShell(m_NumberOfNodes, UNSET_K_CORE);
	std::vector<unsigned int>  Degrees(m_NumberOfNodes, 0);
	for (unsigned int NodeID = 0; NodeID < m_NumberOfNodes; ++NodeID)	{
		Degrees[NodeID] =static_cast<unsigned int>(m_Offsets[NodeID + 1] - m_Offsets[NodeID]);
		if (Degrees[NodeID] == 0) { KShell[NodeID] = 0; }
	}
	unsigned short CurrentShell = 1; 
	bool NodesInShell = false; 
	do
	{
		NodesInShell = false;
		bool any_degree_changed = false;
		do {
			any_degree_changed = false;
			for (unsigned int NodeID = 0; NodeID < m_NumberOfNodes; ++NodeID)
			{

				if (KShell[NodeID] == UNSET_K_CORE && Degrees[NodeID] <= CurrentShell)
				{
					KShell[NodeID] = CurrentShell;
					NodesInShell = true;
					for (auto p = m_Graph + m_Offsets[NodeID]; p < m_Graph + m_Offsets[NodeID + 1]; ++p)
					{
						if (KShell[*p] == UNSET_K_CORE)
						{
							--Degrees[*p];
							any_degree_changed = true;
						}
					}
				}
			}
		} while (any_degree_changed);
		++CurrentShell;
	} while (NodesInShell);
	return KShell; 
}

/*
	Check wether q is a neighbor of p (i.e. if there exists an edge p -> q)
	For an undirected graph, the order does not matter.
	Input: the two nodes to check
	Output: wether there is an edge p->q
	Note: we are working under the assumption that the list of p's neighbors is ordered,
		and so we use binary search.
		Usage of the binary search keeps the proccess to O(log(V))
*/
bool GraphSnapshot::areNeighbors(const unsigned int p, const unsigned int q)
{

	unsigned int first = m_Offsets[p],  //first array element
		last = m_Offsets[p+1] - 1,     //last array element
		middle;                       //mid point of search

	while (first <= last)
	{
		middle = (first + last) / 2; //this finds the mid point
		if (m_Graph[middle] == q) {
			return true;
		}
		else if (m_Graph[middle]> q) // if it's in the lower half
		{
			last = middle - 1;
		}
		else {
			first = middle + 1;      //if it's in the upper half
		}
	}
	return false;  // not found
}

template <typename  value_type> bool GraphSnapshot::SaveValueToFile(const std::vector<value_type>& values, const std::string& FileName, bool IsNewFile)
{
	FILE* hFile = std::fopen(FileName.c_str(), (IsNewFile) ? "w+b" : "ab");
	std::fwrite(&values[0], sizeof(value_type), values.size(), hFile);
	std::fclose(hFile); hFile = NULL;
	return true;
}
template bool GraphSnapshot::SaveValueToFile<unsigned int>(const std::vector<unsigned int>& values, const std::string& FileName, bool IsNewFile);
template bool GraphSnapshot::SaveValueToFile<float>(const std::vector<float>& values, const std::string& FileName, bool IsNewFile);
template bool GraphSnapshot::SaveValueToFile<unsigned __int64>(const std::vector<unsigned __int64>& values, const std::string& FileName, bool IsNewFile);
template bool GraphSnapshot::SaveValueToFile<unsigned short>(const std::vector<unsigned short>& values, const std::string& FileName, bool IsNewFile);
/*
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS> MyGraph;
typedef boost::graph_traits<MyGraph>::vertex_descriptor MyVertex;
void GraphSnapshot::ToBoostGraph() const
{
	MyGraph G;
	for (unsigned int NodeID = 0; NodeID < m_NumberOfNodes; ++NodeID) {
		//http://www.boost.org/doc/libs/1_64_0/libs/graph/doc/leda_conversion.html
//		boost::add_edge(0, 1, g);
//		boost::add_edge(0, 2, g);
//		boost::add_edge(1, 2, g);
//		boost::add_edge(1, 3, g);
	}

}

*/