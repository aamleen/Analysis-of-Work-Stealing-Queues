from random import randint

numNodes = 200
numConstraints = 2*(10**4)

# (10**4, 10**4)
# (200, 2*(10**4))

constraints = set()

while (len(constraints)!=numConstraints):
	v1 = randint(1, numNodes)
	
	v2 = randint(1, numNodes)

	while (v2 == v1):
		v2 = randint(1, numNodes)

	cons = randint(0, 3)

	constraints.add((v1, v2, cons))

print(numNodes)
print(numConstraints)

for i in range(numConstraints):
	c = constraints.pop()
	print(str(i+1) +  ",", str(c[0]) + ",", str(c[1]) + ",", str(c[2]) + ",", 0)
