import sys
import subprocess
import re
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from pylab import *

class Test():
	def __init__(self, x, y):
		fig = plt.figure(1)
		ax = fig.add_subplot(111)
		ax.plot(x, y, 'D', color='red')
		ax.set_xbound(64,1472)
		ax.set_ybound(0,400)
		plt.savefig("test1.png")

def ping_handle(size):
	#print [2]
	trace_file = open('trace_file','r')
	values = []
	fvalues = []
	for each_line in trace_file.readlines():
		#print [3]
		match = re.search(r'time=(.*)ms',each_line)
		if (match !=None):
			#print [4]
			a = match.groups()
			values.append(a[0])
	#print values
	#for i in range(0,len(values),1):
		#test1 = Test(size,values[i])
	fvalues =  map(float,values)
	#print fvalues
	#print min(fvalues)
	return min(fvalues)
  
  

sortt = []
msgsiz = []
for size in range(32,1472,32):
	subprocess.call("hrping -n 64 -l "+str(size)+" 74.125.140.106 > trace_file",shell = True)
	#print [1]
	b = ping_handle(size)
	sortt.append(b)
	msgsiz.append(size)
	#print sortt
print msgsiz
print sortt
plot(msgsiz, sortt, 'D')
xlabel('size of ping msg')
ylabel('Smallest observed RTT')
title('slope gives BW, Y intercept gives latency')
grid(True)
savefig("slope.png")
show()

