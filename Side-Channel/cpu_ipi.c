#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/unistd.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <asm/apic.h>
#include <asm/xen/events.h>
#include <asm/xen/hypercall.h>
#include <asm/xen/hypervisor.h>



#define INTERVAL 180
#define INTERVAL_CYCLE 100000
#define IPI_ADDR 0xffffffff813af770


#ifdef __i386
__inline__ uint64_t rdtsc(void) {
	uint64_t x;
	__asm__ volatile ("rdtsc" : "=A" (x));
	return x;
}
#elif __amd64
__inline__ uint64_t rdtsc(void) {
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d<<32) | a;
}
#endif

#define SEND_CPUID 0
#define RECV_CPUID 0
void (*xen_send_IPI_one)(unsigned int cpu, enum ipi_vector vector) = IPI_ADDR;

int cpu_ipi(void *ptr){
	unsigned long next_time;
	uint64_t tsc;

	set_cpus_allowed_ptr(current, get_cpu_mask(SEND_CPUID));
	next_time = jiffies + INTERVAL*HZ;

	while (time_before(jiffies, next_time)) {
		tsc = rdtsc() + INTERVAL_CYCLE;
/* KVM invoke IPI */
//		apic->send_IPI_mask(get_cpu_mask(RECV_CPUID), CALL_FUNCTION_VECTOR);
/** Xen invoke IPI */
		xen_send_IPI_one(RECV_CPUID, XEN_CALL_FUNCTION_VECTOR);
		while (rdtsc() < tsc);
	}
	
	printk("CPU INTERRUPTION DONE!\n");

	return 0;
}


static int __init cpu_ipi_init(void){
	struct task_struct *cpu_ipi_task;

	printk("entering cpu_ipi module\n");
	cpu_ipi_task = kthread_run(&cpu_ipi, NULL, "CPU INTERRUPTION");

	return 0;
}

static void __exit cpu_ipi_exit(void){
	printk("leaving cpu_ipi module\n");
}

module_init(cpu_ipi_init); 
module_exit(cpu_ipi_exit);

MODULE_LICENSE("GPL");
