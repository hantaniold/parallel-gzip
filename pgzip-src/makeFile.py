#Make a big file
import sys

text = open(sys.argv[1],"r").read()
outFile = open(sys.argv[2],"a")
nrCopies = int(sys.argv[3])

while nrCopies > 0:
	outFile.write(text)
	#print nrCopies
	nrCopies -= 1

