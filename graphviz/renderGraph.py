from graphviz import Digraph

with open("DFA.txt", 'r') as file:
	graph = Digraph("DFA", format = "png", strict = True)
	graph.attr('node', shape = 'plaintext')
	graph.node(' ')
	graph.attr('node', shape = 'doublecircle')
	final = file.readline().split(' ')
	for itr in final[: -1]:
		graph.node(itr)
	graph.attr('node', shape = 'circle')
	graph.edge(' ', '0')
	for line in file:
		try:
			state1, transition, state2 = line.split(' ')
			graph.edge(state1, state2, label = transition)
		except:
			pass
	graph.view()