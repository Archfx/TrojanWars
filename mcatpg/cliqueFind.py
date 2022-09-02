from igraph import Graph

graphIn = open('graph.g', 'r')
# count = 0

Found = True
numNodes = 0
edges = []
for line in graphIn:
    if Found :
        numNodes = int(line.strip())
        Found = False

    if "," in line:
        edges.append([int(line.split(",")[0]), int(line.split(",")[1])])
graphIn.close()
# print(edges)
g = Graph(n = numNodes, edges=edges)

maxCliques = g.maximal_cliques()
maxCliques = sorted( maxCliques, key=len , reverse=True)

f = open("cliques.g", "w")
for x in maxCliques:
    f.write(" ".join(map(str, x))+"\n")
f.close()

