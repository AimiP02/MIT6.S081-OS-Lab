#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_wolfie(void)
{

  char img[] = "\
LGLfjjjtttjt;;;;;;;;,,,,,,,;,;;;;i;;;tftttjjLGGLff\n\
LLGftiiiiii,,;;;,;;,,,,,,,,,,,,,;;;;;,;fiitjLGLfff\n\
fLLftiiii;,,,;,,;,,,,,,,,,,,,,,,,;;,;,,,fitjLLLLff\n\
LLLLjtiti,,,;,,,,,;,,,,,,,,,,,,,,,;,,;,,,jjfLLLLLL\n\
LLGGLfff,;,;;,,,,,;,,,,,,;,,,,,,,,,;,,;,,,LLGLLLGG\n\
GDDDGGG;;,,,,,,,,;,,,,,,,;,,,,,,,,,,;,,;,,;LDDGGDD\n\
KKKKEE;,,,;,,,,,,;,,,,,,,,,,,,,,,,,,;,,,,,,;EKKKKK\n\
KKKKEt,,,,;,,,,,,i,:::,,,:,,,,,,,:,:,,,,,,,,fKKKKK\n\
DDDDG,,,,;;,,,::,i,:::::::;:::::::,::,,,,,,,,DDDGD\n\
LLDKi,,,,;,,::::,i,:::::::;::::::::,::,,,;,,,iLLLL\n\
fDWG,,,,,,,,,:::,i,:::::::,:::,,,,:,::,,,,,,,,Lfff\n\
fLWt,,,,;,,,,:::,t,:::::,,,,:,,,,,,,,,,,,,;,,,ifff\n\
ffE;,,,,;,,,,,::,t,,,,,,,,,,:,,,,,,:,,,,,,;,,,;Lff\n\
fff,,,,,;,,,,,:,,j,,,,,,,,,;:,,;,,,,,,,,,,,;,,,Lff\n\
LEt,,,,,;,,,,,,,,f,,,,,,,,,;,,,,,,,,,,,,,,,,,,,iff\n\
WKi;,,,;;,,,,,,,,ji,,,,,,,,,,,,,;,,,,,,,,,,,,,,;Lf\n\
KDi,,,,,;,,,,,,:t;j,,,,,,,;,i,,,i,,,,,,,,,,;,,,;Lf\n\
Kfi;,,,;;,,,,,,,j:;,,,:,:,,,,i,,t,,,,,,,,,,;,,,,fL\n\
Wii,,,,;;,,,,,,t:::j,,::::,;,j,,jt,;,,,,,,,;,,,;tG\n\
K;i,,,,;i,;,,,i;::.:i::,:,,,i,;,ji,,,,,,,,,i;;,;iE\n\
K;i,,,;;ii,,,it:...:t,,:::,,t,j,L:j,i,,,,,,i;;;;;E\n\
Dii;,,;,f;,,it,::::.:j:,,::t,f,i,::i,j,,,;,i,;;;;D\n\
L;i;;,;,j,,jt,,,ii;,::t,:::i,;tj::;ttjGi;t;ti;;,iG\n\
f;;;;,,,jtj;:::.:.::;;,i::,,i,jjf,::,;ij;,tit;;;if\n\
ti;;,;;,jt,:::,;;,,i;::;t,:,t,;jtfji;,jttfjit;,;tj\n\
jiii,;;,i,:,jEWWKWKKf.::;t,,:;j;GGKKKGjjiif;;;;itj\n\
jtj;;;,,,LGEEEDGjjttji.:.,j,:tiLLLDEEKWWEjii;;;ttt\n\
jjj;;;;;;f:::::jiiiit:::.:tf,,;:::jttttLfiij,;;tjt\n\
tjtti;;;;t:::::iiii;i..:.:ttLii:::jiiiit::ft;;ttjt\n\
tfjtt;;;;;i::::,i;;;i:..::,i::i,::t;iiii::L;;tttft\n\
tLtttt;;;tj:::::i,,;;:...::...:..:,;;;;,::j;itttGj\n\
jGttttti;;ti:::::;i,:....:.....::::i;;;::iitttttDL\n\
GDfttttti;if;::::::::....::....::::::::::jttttttDG\n\
DDGttttttttif::::::::.....;......:::::::,ftttttjDD\n\
GGGjtttttttttf:::::.......:.......::::::;tttjttfGG\n\
LLffjtttjjttttj::.:...............::::::ftttttjLGG\n\
LG;ftjtttjLjttij::...:............:::::,tttjtttLLL\n\
Lf,ffjjttttjLLjt;::.................:::fttfttfjLLL\n\
Li;jLjjftttttf::...................:::;ttfttjtLfff\n\
f,ijjLtfftttttj::.::.....::.......::::ttftttffffff\n\
;;jjjjLtLLititt::..:......::...:..:::,iitttGttffff\n\
,;fjjjjLjLLtiiii:::.............:::::tfjttffG,fjjj\n\
,tjjjjjjffj.fttt:::..:...........:::j:LtttGfj;jjjj\n\
,jjjjjjjjjt.jfit;::::::........::::tjLEjtLffj;tttt\n\
ijjjjjjjjtt ttftjt,:::....:...:::ijjjjjtjfjjj;iiii\n\
jfjjjjjjjtt itjjjj;i,:.::..::::ijjjjjjftLfjjj;;;;;\n\
jjjjjjjjjjt ,jjjjj;,;i;:::::,i;jjjjjjjftffjjj;;;;;\n\
\n\
";

  void *buf;
  uint size;

  if((argptr(0, (void*)&buf, sizeof(*buf))) < 0 || (argint(1, (int*)&size)) < 0) {
    return -1;
  }

  if(size < sizeof(img)) {
    return -1;
  }

  strncpy((char*)buf, img, sizeof(img));

  return sizeof(img);
}