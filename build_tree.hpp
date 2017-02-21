#include <iostream>
#include <fstream>
#include <algorithm>
#include <map>
#include "sparql.hpp"
class triple{
public:
	int subject;
	int predicate;
	int object;

	void save(graphlab::oarchive& oarc) const {
   		oarc << subject << predicate << object;
  	}

  	void load(graphlab::iarchive& iarc) {
    	iarc >> subject >> predicate >> object;
  	}
};

class node{
public:
	int node_value;
	int father_node_value;
	int distance_to_root;
	int edge_value;
	int edge_dir;
	bool is_leaf;

	void save(graphlab::oarchive& oarc) const {
    	oarc << node_value << father_node_value << distance_to_root << edge_value << edge_dir << is_leaf;
  	}
 
 	void load(graphlab::iarchive& iarc) {
    	iarc >> node_value >> father_node_value >> distance_to_root >> edge_value >> edge_dir >> is_leaf;
  	}
};

class sparql_to_tree{
private:
std::vector<node> get_leaves(std::vector<triple> & trps, node & the_node, int distance){
	std::vector<node> ret;
	for(unsigned int m = 0; m < trps.size(); ++ m){
		if(trps[m].subject == the_node.node_value && the_node.father_node_value != trps[m].object){
			node ret_ele;
			ret_ele.node_value = trps[m].object; ret_ele.father_node_value = the_node.node_value; ret_ele.distance_to_root = distance;
			ret_ele.edge_value = trps[m].predicate; ret_ele.edge_dir = DIR_IN;
			ret.push_back(ret_ele);
		}
		else if(trps[m].object == the_node.node_value && the_node.father_node_value != trps[m].subject){	
			node ret_ele;
			ret_ele.node_value = trps[m].subject; ret_ele.father_node_value = the_node.node_value; ret_ele.distance_to_root = distance;
			ret_ele.edge_value = trps[m].predicate; ret_ele.edge_dir = DIR_OUT;
			ret.push_back(ret_ele);
		}
	}
	return ret;
}

int get_distance_sum(std::vector<std::vector<node> > & tree){
	int ret = 0;
	for(unsigned int i = 0; i < tree.size(); ++ i){
		for(unsigned int j = 0; j < tree[i].size(); ++ j){
			ret += tree[i][j].distance_to_root;
		}
	}
	return ret;
}
public:

	tree_sparql get_tree_sparql(const char * file_name){
	std::map<int, int> id_val_map;
	std::vector<triple> trps;	
	std::ifstream in(file_name, std::ifstream::in);
	int a,b,c;	

	while(in >> a >> b >> c){
		triple trp;
		trp.subject = a; trp.predicate = b; trp.object = c;
		trps.push_back(trp);
	}
	//std::cout << "triple_size: " << trps.size() << std::endl;
	/*
	std::cout << "*$*$*$*$*$*$*$*$*$*\n";

	for(unsigned int i = 0; i < trps.size(); ++ i){
		std::cout << trps[i].subject << " " << trps[i].predicate << " " << trps[i].object << std::endl;
	}

	std::cout << "^%^%^%^%^%^%^%^%^\n";*/

	//构造候选根节点序列
	std::vector<int> root_nodes;
	for(unsigned int i = 0; i < trps.size(); ++ i){
		if(std::find(root_nodes.begin(), root_nodes.end(), trps[i].subject) == root_nodes.end())
			root_nodes.push_back(trps[i].subject);
		if(std::find(root_nodes.begin(), root_nodes.end(), trps[i].object) == root_nodes.end())
			root_nodes.push_back(trps[i].object);
	}

	//std::cout << "根节点构造完毕" << std::endl;

	std::vector<std::vector<node> > tree;
	std::vector<node> root_level;
	std::vector<std::vector<int> > product_plan;

	std::vector<std::vector<std::vector<node> > > trees;
	std::vector<std::vector<std::vector<int> > > product_plans;
	//以每个点为根节点
	for(unsigned int i = 0; i < root_nodes.size() ; ++ i){
		tree.clear();
		root_level.clear();
		product_plan.clear();

		node root_node;
		root_node.node_value = root_nodes[i];
		root_node.father_node_value = -1;
		root_node.distance_to_root = 0;
		root_node.edge_value = NONE;
		root_node.edge_dir = NONE;
		root_level.push_back(root_node);
		tree.push_back(root_level);
		int last_level = 0;
		
		while(true){
			int update_count = 0;
			std::vector<node> the_level;
			for(unsigned int j = 0; j < tree[last_level].size(); ++ j){
				//获取这一层里的每一个点的第一层叶子节点
				std::vector<node> leaves = get_leaves(trps, tree[last_level][j], last_level + 1);
				std::vector<int> single_plan;
				update_count += leaves.size();
				if(leaves.size() == 0){
					tree[last_level][j].is_leaf = true;
				}
				else{
					tree[last_level][j].is_leaf = false;
				}
				for(unsigned int k = 0; k < leaves.size(); ++ k){
					the_level.push_back(leaves[k]);
					single_plan.push_back(leaves[k].node_value);
				}
				
				if(leaves.size() != 0){
					single_plan.push_back(tree[last_level][j].node_value);
					product_plan.push_back(single_plan);
				}
				
			}
			if(update_count == 0){
				for(unsigned int k = 0; k < tree[last_level].size(); ++ k){
					tree[last_level][k].is_leaf = true;
				}
				break;
			}
			tree.push_back(the_level);
			++ last_level;
		}

/*		for(unsigned int p = 0 ; p < tree.size(); ++ p){
			for(unsigned int q = 0; q < tree[p].size(); ++ q){
				std::cout << tree[p][q].node_value << "(" << tree[p][q].father_node_value << ")" << "[" << tree[p][q].distance_to_root << "] ";
			}
			std::cout << std::endl;
		}
		std::cout << "level: " << tree.size() << std::endl;
		std::cout << "plan:" << std::endl;

		for(unsigned int p = 0; p < product_plan.size(); ++ p){
			for(unsigned int q = 0; q < product_plan[p].size(); ++ q){
				std::cout << product_plan[p][q] << " ";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;*/
		
		trees.push_back(tree);
		product_plans.push_back(product_plan);
	}

	//std::cout << "候选树构造完毕 个数: " << trees.size()  << std::endl;

	int min_level = trees[0].size(); int min_sum = get_distance_sum(trees[0]); int selected_tree = 0;
	
	//std::cout << "初始化选择参数完毕\n";

	for(unsigned int i = 1; i < trees.size(); ++ i){
		int current_level = trees[i].size();
		int current_sum = get_distance_sum(trees[i]);
		if(current_level < min_level){
			selected_tree = i;
			min_level = current_level;
		}
		else if(current_level == min_level && current_sum < min_sum){
			selected_tree = i;
			min_sum = current_sum;
		}
	}

	std::cout << "selected tree: " << std::endl;

	for(unsigned int p = 0 ; p < trees[selected_tree].size(); ++ p){
		for(unsigned int q = 0; q < trees[selected_tree][p].size(); ++ q){
			std::cout << trees[selected_tree][p][q].node_value << "(" << trees[selected_tree][p][q].father_node_value << ")" << "[" << tree[p][q].distance_to_root << "] ";
		}
		std::cout << std::endl;
	}

	std::cout << std::endl;

	tree_sparql ts;
	int id_counter = 0;
	for(unsigned int i = 0; i < trees[selected_tree].size(); ++ i){
		for(unsigned int j = 0; j < trees[selected_tree][i].size(); ++ j){
			single_vertex sv; node nd = trees[selected_tree][i][j]; id_val_map.insert(std::pair<int, int>(nd.node_value, id_counter));
			sv.vertex_id = id_counter ++ ; sv.vertex_value = nd.node_value < 0 ? ALL_MATCH : nd.node_value;
			sv.father_vertex_value = nd.father_node_value < 0 ? ALL_MATCH : nd.father_node_value; 
			sv.edge_value = nd.edge_value < 0 ? ALL_MATCH : nd.edge_value;
			sv.edge_dir = nd.edge_dir; sv.is_leaf = nd.is_leaf;
			ts.vertices.push_back(sv);
		}
	}
	//std::cout << "目标数基本结构构造完成\n";
	std::vector<std::vector<int> > target_plan = product_plans[selected_tree];
	std::reverse(target_plan.begin(), target_plan.end());
	for(unsigned int i = 0; i < target_plan.size(); ++ i){
		for(unsigned int j = 0; j < target_plan[i].size(); ++ j){
			target_plan[i][j] = id_val_map[target_plan[i][j]];
		}
	}
	ts.cartesian_product_plan = target_plan;
	ts.height = trees[selected_tree].size();
	ts.root_id = ts.vertices[0].vertex_id;

	//std::cout << "即将返回结果\n";
	
	return ts;
	}
};
