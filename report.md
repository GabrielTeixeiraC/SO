# PAGINADOR DE MEMÓRIA - RELATÓRIO

1. Termo de compromisso

Os membros do grupo afirmam que todo o código desenvolvido para este
trabalho é de autoria própria.  Exceto pelo material listado no item
3 deste relatório, os membros do grupo afirmam não ter copiado
material da Internet nem ter obtido código de terceiros.

2. Membros do grupo e alocação de esforço

Preencha as linhas abaixo com o nome e o e-mail dos integrantes do
grupo.  Substitua marcadores `XX` pela contribuição de cada membro
do grupo no desenvolvimento do trabalho (os valores devem somar
100%).

  * Gabriel Teixeira Carvalho <gabrieltc@ufmg.br> 50%
  * Gabriel Lima Barros <gabriellimab@ufmg.br> 50%

3. Referências bibliográficas
  * Fundamentos de Sistemas Operacionais - Silberschatz, Galvin e Gagne
  * Biblioteca "signal.h" - https://www.tutorialspoint.com/c_standard_library/signal_h.htm
  * Biblioteca "time.h" - https://www.tutorialspoint.com/c_standard_library/time_h.htm
  * Biblioteca "ucontext.h" - https://pubs.opengroup.org/onlinepubs/7908799/xsh/ucontext.h.html
  * Biblioteca "unistd.h" - https://pubs.opengroup.org/onlinepubs/007908775/xsh/unistd.h.html

4. Estruturas de dados

  1. Descreva e justifique as estruturas de dados utilizadas para
     gerência das threads de espaço do usuário (partes 1, 2 e 5).

      Para gerenciar as threads de espaço do usuário, utilizamos uma lista encadeada de threads. Cada thread possui um identificador, um contexto, um estado e um ponteiro para a próxima thread. O contexto é um ponteiro para uma estrutura ucontext_t, que contém o contexto da thread. O estado é um inteiro que indica se a thread está em execução, pronta ou bloqueada. O ponteiro para a próxima thread é um ponteiro para a próxima thread na lista encadeada.

      Para fazer uma thread esperar por outra, utilizamos uma lista encadeada de threads em estado de espera. As threads que são colocadas em estado de espera tem contém em sua estrutura de dados um ponteiro para a thread pela qual ela está esperando e é bloqueada até que a thread pela qual ela está esperando seja finalizada.

      Ao terminar a execução de uma thread a função dccthread_exit percorre toda a lista de threads em espera e libera as threads que estão esperando pela thread que está sendo finalizada. A função dccthread_exit também libera a memória alocada para a thread que está sendo finalizada.

      A utilização de uma lista encadeada de threads para gerenciar as threads de espaço do usuário foi escolhida por ser uma estrutura de dados simples e eficiente para o gerenciamento de threads.


  2. Descreva o mecanismo utilizado para sincronizar chamadas de
     dccthread_yield e disparos do temporizador (parte 4).

      Para sincronizar as chamadas de dccthread_yield utilizamos a função sigprocmask da biblioteca signal.h. A função sigprocmask bloqueia o sinal SIGALRM, que é o sinal que é disparado pelo temporizador. A função sigprocmask é chamada antes de dccthread_yield e é desbloqueada após a chamada de dccthread_yield.

      Para sincronizar os disparos do temporizador utilizamos a função setitimer da biblioteca time.h. A função setitimer é chamada na função dccthread_init e é utilizada para disparar o sinal SIGALRM a cada 10000000 microssegundos.