import sys
import os

# Leave the path changes here!!!
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..'))

from src.accelerated_graph_features.test_python_converter import create_graph
from src.accelerated_graph_features.feature_wrappers import example_feature, clustering_coefficient, k_core, \
    node_page_rank, bfs_moments, motif

import networkx as nx

from contextlib import contextmanager


@contextmanager
def silence_stdout():
    new_target = open(os.devnull, "w")
    old_target = sys.stdout
    sys.stdout = new_target
    try:
        yield new_target
    finally:
        sys.stdout = old_target


def test_features():
    g = create_graph(3, GraphType=nx.DiGraph)
    with silence_stdout():
        example_feature(g)
        clustering_coefficient(g)
        k_core(g)
        node_page_rank(g)
        bfs_moments(g)
        m_res = motif(g, level=3)
    print(m_res)
    for node, node_list in enumerate(m_res):
        print(node)
        for k, v in enumerate(node_list):
            if v != 0:
                print('\t{}:{}'.format(k, v))
    print(motif(g,level=4,gpu=True))

if __name__ == '__main__':
    test_features()
