from postman_problems.solver import cpp
from postman_problems.stats import calculate_postman_solution_stats

# find CPP solution
circuit, graph = cpp(edgelist_filename='edges.csv')

# print solution route
with open("output.txt", "a") as f:
    for e in circuit:
        print(e[0].replace('/', ','), e[1].replace('/', ','))
        f.write(e[0].replace('/', ','))
        f.write(" ")
        f.write(e[1].replace('/', ','))
        f.write("\n")

# print solution summary stats
for k, v in calculate_postman_solution_stats(circuit).items():
    print(k, v)
