#include <graphlab.hpp>
#include "define.h"
#include <vector>
class single_vertex{
public:
	int vertex_id;
	int vertex_value;
	int father_vertex_value;
	int edge_value;
	int edge_dir;
	bool is_leaf;

	void save(graphlab::oarchive& oarc) const {
    	oarc << vertex_id << vertex_value << father_vertex_value << edge_value << edge_dir << is_leaf;
  	}
	
	void load(graphlab::iarchive& iarc) {
    	iarc >> vertex_id >> vertex_value >> father_vertex_value >> edge_value >> edge_dir >> is_leaf;
	}

};

class match_answer{
public:
	int id;
	int candidate;

	match_answer () {}
	match_answer(int e_id, int e_candidate) : id(e_id), candidate(e_candidate) {}
	
	void save(graphlab::oarchive& oarc) const {
    	oarc << id << candidate;
  	}
	
	void load(graphlab::iarchive& iarc) {
    	iarc >> id >> candidate;
	}

};

class tree_sparql{
public:
	std::vector<single_vertex> vertices;
	std::vector<std::vector<int> > cartesian_product_plan;
	int height;
	int root_id;

	void save(graphlab::oarchive& oarc) const {
    	oarc << vertices << cartesian_product_plan << height << root_id;
  	}
	
	void load(graphlab::iarchive& iarc) {
    	iarc >> vertices >> cartesian_product_plan >> height >> root_id;
	}

	void init_test_sparql(){

		root_id = 0;
		
		single_vertex sv;
		//根节点 0
		sv.vertex_id = 0; sv.vertex_value = ALL_MATCH; sv.father_vertex_value = NONE; 
		sv.edge_dir = NONE; sv.edge_value = NONE; sv.is_leaf = false; 
		vertices.push_back(sv);

		//1
		sv.vertex_id = 1; sv.vertex_value = B; sv.father_vertex_value = ALL_MATCH;
		sv.edge_dir = DIR_OUT; sv.edge_value = A; sv.is_leaf = true;
		vertices.push_back(sv);

		//2
		sv.vertex_id = 2; sv.vertex_value = D; sv.father_vertex_value = ALL_MATCH;
		sv.edge_dir = DIR_IN; sv.edge_value = C; sv.is_leaf = false;
		vertices.push_back(sv);

		//3
		sv.vertex_id = 3; sv.vertex_value = ALL_MATCH; sv.father_vertex_value = ALL_MATCH;
		sv.edge_dir = DIR_IN; sv.edge_value = G; sv.is_leaf = false;
		vertices.push_back(sv);

		//4
		sv.vertex_id = 4; sv.vertex_value = ALL_MATCH; sv.father_vertex_value = D;
		sv.edge_dir = DIR_IN; sv.edge_value = E; sv.is_leaf = true;
		vertices.push_back(sv);

		//5
		sv.vertex_id = 5; sv.vertex_value = F; sv.father_vertex_value = D;
		sv.edge_dir = DIR_OUT; sv.edge_value = ALL_MATCH; sv.is_leaf = true;
		vertices.push_back(sv);

		//6
		sv.vertex_id = 6; sv.vertex_value = I; sv.father_vertex_value = ALL_MATCH;
		sv.edge_dir = DIR_IN; sv.edge_value = H; sv.is_leaf = true;
		vertices.push_back(sv);		
	
		height = 3;

		std::vector<int> p1, p2, p3;
		p1.push_back(4); p1.push_back(5); p1.push_back(2);
		p2.push_back(6); p2.push_back(3);
		p3.push_back(1); p3.push_back(2); p3.push_back(3); p3.push_back(0);

		cartesian_product_plan.push_back(p1); cartesian_product_plan.push_back(p2); cartesian_product_plan.push_back(p3);
		
	}

	/*bool integrity_check(std::vector<match_answer>& ma){
		for(unsigned int i = 0; i < vertices.size() ; ++ i){
			if(!contains_match_of(vertices[i].vertex_id))
				return false;
		}
		return true;
		}*/
};

class scatter_type{
public:
	
	std::vector<match_answer> match_answers;
	scatter_type operator+=(const scatter_type& st){
		int count = st.match_answers.size();
		for(int i = 0;i < count; ++ i){
			this->match_answers.push_back(st.match_answers[i]);
		}
		return *this;
	}

	void save(graphlab::oarchive& oarc) const {
    	oarc << match_answers;
  	}
  	
	void load(graphlab::iarchive& iarc) {
    	iarc >> match_answers;
	}
	
};
