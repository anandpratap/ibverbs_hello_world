from pylab import *
import re

class Log:
    def __init__(self):
        pass

def reject_outliers(data, m=30):
    return data[abs(data - mean(data)) < m * std(data)]

prop_map = {4:'REG', 3:'DEREG',0:'ALLOC',2:'DEALLOC',1:'DATAT',5:'RUN'}
color_map = {3:'r',6:'g',9:'m',12:'b',15:'y', 18:'c', 21:'b', 24:'b', 27:'b', 30:'k'}
def parse_log(fname):
    print "parsing....", fname,
    log = Log()

    allocation_time = []
    registration_time = []
    deregistration_time = []
    data_time = []
    deallocation_time = []
    total_runtime = []
    nline = 0
    try:
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
    except:
        pass

    if nline > 0:
        print nline
        if(len(total_runtime) == 0):
            nt = nline/5
        else:
            nt = nline/6
#        assert(nt == len(registration_time))
#        assert(nt == len(deregistration_time))
#        assert(nt == len(allocation_time))
#        assert(nt == len(deallocation_time))
#        assert(nt == len(data_time))

    log.registration_time = reject_outliers(array(registration_time[2:]))
    log.deregistration_time = reject_outliers(array(deregistration_time[2:]))
    log.allocation_time = reject_outliers(array(allocation_time[2:]))
    log.deallocation_time = reject_outliers(array(deallocation_time[2:]))
    log.data_time = reject_outliers(array(data_time[2:]))

    try:
        log.total_runtime = reject_outliers(array(total_runtime[2:]))
    except:
        log.total_runtime = array(total_runtime)
    print "..completed!"
    return log

def plots(log, fignum, mode, p):
    reg_time = log.registration_time
    dereg_time = log.deregistration_time

    a_time = log.allocation_time
    da_time = log.deallocation_time

    d_time = log.data_time
    t_time = log.total_runtime


    low_values_indices = d_time > 1  # Where values are low
    d_time[low_values_indices] = d_time[0]
    MODES = ['S-R', 'WRITE', 'READ']
    plot = semilogy
    figure(fignum)
    subplot(2,3,1)
    title("REG TIME")
    plot(reg_time, label=p + MODES[mode])
    subplot(2,3,2)
    title("DEREG TIME")
    plot(dereg_time, label=p + MODES[mode])
    subplot(2,3,3)
    title("ALLOCATE")
    plot(a_time, label=p + MODES[mode])
    subplot(2,3,4)
    title("DEALLOCATE")
    plot(da_time, label=p + MODES[mode])
    subplot(2,3,5)
    title("DATA")
    plot(d_time, label=p + MODES[mode])
    subplot(2,3,6)
    title("TOTAL Client")
    plot(t_time, label=p + MODES[mode])
    legend(loc=2)

if __name__ == "__main__":
    max_iters = [100]# range(100, 1001, 100)
    nps = [2,3] #[2,3,4]
    modes = [0,1,2]#[0,1,2]
    msg_sizes = range(3, 31, 3)
    msg_sizes = msg_sizes[:]
    print msg_sizes
    figure(11)
    for max_iter in max_iters:
        _c_mean = []
        _c_std = []
        _s_mean = []
        _s_std = []
        for np in nps:
            c_mean = []
            s_mean = []
            c_std = []
            s_std = []
            for mode in modes:
                _c = []
                __c = []
                _s = []
                __s = []
                for msg_size in msg_sizes:
                    c_ = []
                    s_ = []
                    c__ = []
                    s__ = []
                    filename_s = "../../bin/logs/logs_Oct29_2015/server_%i_%i_%i_%i_%i.log"%(0, np, 2**msg_size, mode, max_iter)
                    log_s = parse_log(filename_s)
                    for count, obj in enumerate(dir(log_s)):
                        if not obj.startswith('__'):
                            mean_ = mean(getattr(log_s, obj))
                            std_ = std(getattr(log_s, obj))
                            s_.append(mean_)
                            s__.append(std_)
                            print count, obj
                            subplot(3,2,count-2)
                            color = color_map[msg_size]
                            title(obj)
                            if(mode == 0):
                                semilogy(getattr(log_s, obj)*1e6, label='(size, mode)%i %i'%(msg_size, mode), color=color)
                            else:
                                semilogy(getattr(log_s, obj)*1e6, '--', label='(size, mode)%i %i'%(msg_size, mode), color=color)
                            
                            
                    for n in range(1, 2):
                        filename_c = "../../bin/logs/logs_Oct29_2015/client_%i_%i_%i_%i_%i.log"%(n, np, 2**msg_size, mode, max_iter)
                        log_c = parse_log(filename_c)
                        for count, obj in enumerate(dir(log_c)):
                            if not obj.startswith('__'):
                                mean_ = mean(getattr(log_c, obj))
                                std_ = std(getattr(log_c, obj))
                                c_.append(mean_)
                                c__.append(std_)
                                print count, obj
                                subplot(3,2,count-2)
                                color = color_map[msg_size]
                                title(obj)
                                if(mode == 0):
                                    semilogy(getattr(log_c, obj)*1e6,'x-',label='(size, mode)%i %i'%(msg_size, mode), color=color)
                                else:
                                    semilogy(getattr(log_c, obj)*1e6, '.--', label='(size, mode)%i %i'%(msg_size, mode), color=color)

                    _s.append(s_)
                    __s.append(s__)
                    _c.append(c_)
                    __c.append(c__)

                s_mean.append(_s)
                s_std.append(__s)
                c_mean.append(_c)
                c_std.append(__c)
            _s_mean.append(s_mean)
            print c_mean
            _c_mean.append(c_mean)
            _c_std.append(c_std)
            _s_std.append(s_std)
 
    legend()
    s_mean = array(_s_mean)
    s_std = array(_s_std)
    print shape(_c_mean)
    msg_sizes = 2**array(msg_sizes)
    figure(1)
    for i in range(len(modes)):
        for j in range(6):
            for k in range(len(nps)):
                figure(j)
                title('%s'%prop_map[j])
                if i == 0:
                    color = 'r'
                elif i == 1:
                    color = 'g'
                else:
                    color = 'b'
                if k==0:
                    if(i==0):
                        plot(msg_sizes, s_mean[k,i,:,j]*1e6, '-',color=color, label='server 1 client-server')
                    else:
                        plot(msg_sizes, s_mean[k,i,:,j]*1e6, '-',color=color)
                else:
                    if(i==0):
                        plot(msg_sizes, s_mean[k,i,:,j]*1e6, '--',color=color, label='server 2 client-server')
                    else:
                        plot(msg_sizes, s_mean[k,i,:,j]*1e6, '--',color=color)

    
    c_mean = array(_c_mean)
    c_std = array(_c_std)
    for i in range(len(modes)):
        for j in range(6):
            for k in range(len(nps)):
                figure(j)
                title('%s'%prop_map[j])
                if i == 0:
                    color = 'r'
                elif i == 1:
                    color = 'g'
                else:
                    color = 'b'
                print i, j, k
                if k==0:
                    if(i==0):
                        plot(msg_sizes, c_mean[k,i,:,j]*1e6, 'o',color=color, label='client 1 client-server')
                    else:
                        plot(msg_sizes, c_mean[k,i,:,j]*1e6, 'o',color=color)
                        
                else:
                    if(i==0):
                        plot(msg_sizes, c_mean[k,i,:,j]*1e6, '*', color=color, label='client 2 clients-server')
                    else:
                        plot(msg_sizes, c_mean[k,i,:,j]*1e6, '*', color=color)
                   
    mpi_sizes = [1048576, 2097152, 4194304, 8388608,16777216,33554432,67108864,134217728,268435456,536870912,1073741824]
    mpi_times_new = [33512.57904433, 36789.91715424, 37512.89215870, 39193.10797937, 43696.60606608, 61465.10899998, 79473.04705158, 133686.21515110, 241519.91587132, 424407.99600445,869308.39600973]
    mpi_times = [35538.61900000,35499.75300000,36951.82400000,40006.01500000,44226.98100000,64414.81900000,106530.10900000,164111.09500000,301231.02600000,606884.98200000,1209099.7710]
    for j in range(6):
        figure(j)
        if j==1:
            plot(mpi_sizes, mpi_times, 'k-', label='MPI OLD', linewidth=2)
            plot(mpi_sizes, mpi_times_new, 'k--', label='MPI NEW', linewidth=2)
        legend(loc=2)
        title('red - send_receive, green -> write, blue -> read')
        xlabel('Size in bytes')
        ylabel(r'time in $\mu$ s')
        plt.yscale('log', nonposy='clip')
        plt.xscale('log', nonposy='clip', basex=2)
        savefig('%i.pdf'%j)
    show()


