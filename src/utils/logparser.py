from pylab import *
import re

class Log:
    def __init__(self):
        pass

def reject_outliers(data, m=2):
    return data[abs(data - np.mean(data)) < m * np.std(data)]
def parse_log(fname):
    log = Log()

    allocation_time = []
    registration_time = []
    deregistration_time = []
    data_time = []
    deallocation_time = []
    total_runtime = []
    nline = 0
    with open(fname) as f:
        for line in f:
            nline += 1
            if(re.findall('\\bREG\\b', line)):
                registration_time.append(float(line.split(":")[1]))
            elif(re.findall('\\bDEREG\\b', line)):
                deregistration_time.append(float(line.split(":")[1]))
            elif(re.findall('\\bALLOCATION\\b', line)):
                allocation_time.append(float(line.split(":")[1]))
            elif(re.findall('\\bDEALLOCATION\\b', line)):
                deallocation_time.append(float(line.split(":")[1]))
            elif(re.findall('\\RUNTIME\\b', line)):
                total_runtime.append(float(line.split(":")[1]))
            elif(re.findall('\\DATATIME\\b', line)):
                data_time.append(float(line.split(":")[1]))
            else:
                raise ValueError("SOMETHING WENT WRONG")

    if(len(total_runtime) == 0):
        nt = nline/5
    else:
        nt = nline/6
    assert(nt == len(registration_time))
    assert(nt == len(deregistration_time))
    assert(nt == len(allocation_time))
    assert(nt == len(deallocation_time))
    assert(nt == len(data_time))

    log.registration_time = reject_outliers(array(registration_time))
    log.deregistration_time = reject_outliers(array(deregistration_time))
    log.allocation_time = reject_outliers(array(allocation_time))
    log.deallocation_time = reject_outliers(array(deallocation_time))
    log.data_time = reject_outliers(array(data_time))
    log.total_runtime = reject_outliers(array(total_runtime))
    return log

def plots(log, fignum):
    reg_time = log.registration_time
    dereg_time = log.deregistration_time

    a_time = log.allocation_time
    da_time = log.deallocation_time

    d_time = log.data_time
    t_time = log.total_runtime


    low_values_indices = d_time > 1  # Where values are low
    d_time[low_values_indices] = d_time[0]
 
    plot = semilogy
    figure(fignum)
    title("REG TIME")
    subplot(2,3,1)
    plot(reg_time)
    subplot(2,3,2)
    title("DEREG TIME")
    plot(dereg_time)
    subplot(2,3,3)
    title("ALLOCATE")
    plot(a_time)
    subplot(2,3,4)
    title("DEALLOCATE")
    plot(da_time)
    subplot(2,3,5)
    title("DATA")
    plot(d_time)
    subplot(2,3,6)
    title("TOTAL Client")
    plot(t_time)
       

if __name__ == "__main__":
    log = parse_log("../../bin/logs/size_11/mode_0/server.log")
    sizes = array([7, 11, 15, 19])
    x = 2**sizes
    for obj in dir(log):
        if not obj.startswith('__'):
            creg = []
            sreg = []
            for _size in [7, 11, 15, 19]:
                ctmp = []
                stmp = []
                for mode in [0, 1, 2]:
                    log = parse_log("../../bin/logs/size_%s/mode_%s/client.log"%(_size, mode))
                    ctmp.append(mean(getattr(log, obj)))
                    log = parse_log("../../bin/logs/size_%s/mode_%s/server.log"%(_size, mode))
                    stmp.append(mean(getattr(log, obj)))
                creg.append(ctmp)
                sreg.append(stmp)
            creg = array(creg)
            sreg = array(sreg)
            
            figure()
            title(obj)
            print size(x)
            print size(creg)
            print creg
            colors = ['r', 'g', 'b']
            MODES = ['S-R', 'WRITE', 'READ']
            for i in range(3):
                loglog(x, creg[:,i], 'o-', color=colors[i], label=MODES[i])
                loglog(x, sreg[:,i], 'o--', color=colors[i])
            legend(loc=2)
            xlim(x.min(), x.max())
            xlabel('BUFFER SIZE')
            ylabel('MEAN TIME FOR 50 SAMPLES')
            grid()
            savefig(obj + '.pdf')
    show()
