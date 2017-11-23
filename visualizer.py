#! /usr/bin/python3

import importlib.machinery
import pydot


def unique_name():
    unique_name.counter += 1
    return "v" + str(unique_name.counter)


unique_name.counter = 0


def epsilon():
    node = pydot.Node(unique_name())
    node.set("label", "<&epsilon;>")
    node.set("shape", "square")
    return node


def dfs(tree, gr, parser):
    node = pydot.Node(unique_name())
    gr.add_node(node)
    node.set("label", parser.get_symbol_name(tree[0]))
    for child in tree[1]:
        if type(child) is str:
            node.set("shape", "square")
            continue
        n = dfs(child, gr, parser)
        gr.add_edge(pydot.Edge(node, n))
    if len(tree[1]) is 0:
        e = epsilon()
        gr.add_node(e)
        gr.add_edge(pydot.Edge(node, e))
    return node

def main():
    filename = input('Enter parser filename: ')
    parser = importlib.machinery.SourceFileLoader('parser', filename).load_module()
    tree = parser.Parser().parse(parser.TokenStream(input("Enter string: ")))

    gr = pydot.Dot(graph_type='digraph')
    dfs(tree, gr, parser)
    gr.write_png(input("Enter output filename: "))

if __name__ == '__main__':
    main()
