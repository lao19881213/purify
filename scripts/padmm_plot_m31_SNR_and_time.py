import os 
import multiprocessing
import time
import numpy as np 
import pyfits
import glob
import subprocess
import matplotlib.pyplot as plt

image = "30dor_256"
n_tests = 10
input_SNR = 60

def kernel_settings(kernel):
        J = 4
        oversample = 2
        if kernel == "pswf":
                J = 6
        if kernel == "kb_interp":
                oversample = 1.375
                J = 5
        if kernel == "box":
                J = 1
        if kernel == "kb_min":
                oversample = 1.375
                J = 5
	if kernel == "kb_min4":
		oversample = 1.375
		J = 4
		kernel = "kb_min"
	if kernel == "kb_interp4":
		oversample = 1.375
		J = 4
		kernel = "kb_interp"
	
	return J, oversample, kernel
def run_test_padmm((i, kernel, M_N_ratio, start_time, input_SNR, image)):
	time.sleep(start_time)

	J, oversample, kernel = kernel_settings(kernel)

  	os.system("../build/cpp/example/padmm_m31_simulation " + kernel + " " 
      + str(oversample) + " " +str(J) + " " +str(M_N_ratio) + " " + str(i) + " "+str(input_SNR)+ " " + str(image))
  	results_file = "../build/outputs/"+image+"_results_" + kernel + "_" + str(i) + ".txt"
  	results = np.loadtxt(results_file, dtype = str)
  	SNR = results[0]
  	total_time = results[1]
	os.system("rm " + results_file)
  	return [float(SNR), float(total_time)]
def run_test_ms_clean((i, kernel, M_N_ratio, start_time, input_SNR, image)):
	time.sleep(start_time)
	
	J, oversample, kernel = kernel_settings(kernel)
	
  	os.system("../build/cpp/example/padmm_ms_clean_m31_simulation " + kernel + " " 
      + str(oversample) + " " +str(J) + " " +str(M_N_ratio) + " " + str(i) + " " + str(input_SNR)+ " " +str(image))
  	results_file = "../build/outputs/"+image+"_results_" + kernel + "_" + str(i) + ".txt"
  	results = np.loadtxt(results_file, dtype = str)
  	SNR = results[0]
  	total_time = results[1]
	os.system("rm " + results_file)
  	return [float(SNR), float(total_time)]

def run_test_clean((i, kernel, M_N_ratio, start_time, input_SNR, image)):
	time.sleep(start_time)

	J, oversample, kernel = kernel_settings(kernel)

  	os.system("../build/cpp/example/padmm_clean_m31_simulation " + kernel + " " 
      + str(oversample) + " " +str(J) + " " +str(M_N_ratio) + " " + str(i) + " " + str(input_SNR) + " " + str(image))
  	results_file = "../build/outputs/"+image+"results_" + kernel + "_" + str(i) + ".txt"
  	results = np.loadtxt(results_file, dtype = str)
  	SNR = results[0]
  	total_time = results[1]
	os.system("rm " + results_file)
  	return [float(SNR), float(total_time)]

def collect_data(args, results, M_N_ratios, kernel):

	meantempSNR = []
	errortempSNR = []
	meantempTime = []
	errortempTime = []
	for m in M_N_ratios:
		tempSNR = []
		tempTime = []

		for i in range(len(args)):
			if m == args[i][2]:
				if args[i][1] == kernel:
					tempSNR.append(results[i][0])
					tempTime.append(results[i][1])

		tempSNR = np.array(tempSNR)
		tempTime = np.array(tempTime)

		meantempSNR.append(tempSNR.mean())
		errortempSNR.append(tempSNR.std())
		meantempTime.append(tempTime.mean())
		errortempTime.append(tempTime.std())
	return meantempSNR, errortempSNR, meantempTime, errortempTime

def create_plots(args, results, M_N_ratios, name, kernels, colours,legend = []):
	for k in range(len(kernels)):
		meantempSNR, errortempSNR, meantempTime, errortempTime = collect_data(args, results, M_N_ratios, kernels[k])
		plt.errorbar(M_N_ratios, meantempSNR, errortempSNR, fmt='', c = colours[k])
	if len(legend) > 0:
		plt.legend(legend)
	plt.xlabel("M/N")
	plt.ylabel("SNR, db")
	plt.xlim(0, 2.2)
	plt.ylim(5, 60)
	plt.savefig(name + "_SNR_plot.pdf")
	plt.clf()

	for k in range(len(kernels)):
		meantempSNR, errortempSNR, meantempTime, errortempTime = collect_data(args, results, M_N_ratios, kernels[k])
		plt.errorbar(M_N_ratios, meantempTime, errortempTime, fmt='', c = colours[k])
	plt.xlabel("M/N")
	plt.ylabel("Time (seconds)")
	plt.xlim(0, 2.2)
	plt.savefig(name + "_Time_plot.pdf")
	plt.clf()



if __name__ == '__main__':
	M_N_ratios = np.arange(1, 11) * 0.2
	args = []

	test_num = 0
	#kernels = ["kb", "kb_interp", "pswf", "gauss", "box", "gauss_alt", "kb_min", "kb_min4"]#, "kb_interp4"]
	kernels = ["kb", "pswf", "gauss", "gauss_alt"]
	total_tests = n_tests * len(kernels) * len(M_N_ratios)
	for i in range(1, n_tests + 1):
		for k in kernels:
			for m in M_N_ratios:
				test_num = test_num + 1
				args.append((test_num, k, m, test_num * 1./ total_tests * 30., input_SNR, image))
				print test_num
	n_processors = multiprocessing.cpu_count() + 1
	p = multiprocessing.Pool(min(n_processors, 41)) # Limiting the number of processes used to 40, otherwise it will cause problems with the user limit
	
	#legend = ["Kaiser Bessel (KB)", "KB (Linear-interp, Min-oversample)", "PSWF", "Gaussian (Optimal)", "Box", "Gaussian (non-Optimal)", "KB (Min-oversample)", "KB4 (Min-oversample)"]#, "KB4 (Linear-interp, Min-oversample)"]
	#colours = ['blue', 'red', 'black', 'green', 'magenta', 'cyan', 'yellow', "#800000"]#, "#808000"]
	legend = ["Kaiser Bessel (KB)", "PSWF", "Gaussian (Optimal)", "Gaussian (non-Optimal)"]
	colours = ['blue', 'black', 'green', 'cyan']
	results = p.map(run_test_padmm, args)
	create_plots(args, results, M_N_ratios, "padmm", kernels, colours, legend)
	print "PADMM Done!"
	results = p.map(run_test_ms_clean, args)
	create_plots(args, results, M_N_ratios, "ms_clean", kernels, colours)
	print "MS CLEAN Done!"
	results = p.map(run_test_clean, args)
	create_plots(args, results, M_N_ratios, "clean", kernels, colours)
	print "All Done!"
