Код этой лабы с пояснениями лежит у Рязановой на почте, так что брать 1 в 1 наверное опасно. Ниже приведу пояснения из письма.

### Шаблон для генерации (bakery.x):

```
Код bakery.x
```



### Код клиента (bakery_client.с):

```
Код bakery_client.с
```



#### Комментарий

Клиенты создаются с помощью **fork** и подключаются к серверу с разными задержками. Сначала они получают свой номер в очереди (**get_number_1**), а затем обслуживаются (**bakery_service_1**).



### Код сервера (bakery_server.c):

```
Код bakery_server.с
```



#### Комментарий

Сервер предоставляет 2 функции:
- Получение номера (**get_number_1_svc**): каждому приходящему клиенту выдаётся номер - максимальный из выданных + единица.
- Обслуживание клиента по алгоритму Лампорта "Булочная" (**bakery_service_1_svc**).

### Код заглушки сервера (bakery_svc.c):

```
Код bakery_svc.с
```

#### Комментарий
При любом обращении к серверу вызывается функция **bakery_prog_1**. В ней создаётся дополнительный поток для обработки запроса (выполняет функцию **serv_request**). Важно отметить, что после вызова **pthread_create** нигде в программе не вызывается **pthread_join**, поэтому главный поток продолжает своё выполнение параллельно с обработкой запроса, выходя из функции bakery_prog_1 и возвращаясь в цикл ожидания нового запроса. Потоки создаются с атрибутом PTHREAD_CREATE_DETACHED, чтобы их нельзя было присоединить. Из мануала:

```
PTHREAD_CREATE_DETACHED
      Threads that are created using attr will be created in a detached state.

PTHREAD_CREATE_JOINABLE
      Threads that are created using attr will be created in a joinable state.

The default setting of the detach state attribute in  a  newly  initialized  thread  attributes  object  is PTHREAD_CREATE_JOINABLE.
```

**Также в сгенерированный файл bakery.h я добавил строку**:

```c
#define MAX_CLIENTS_COUNT 128
```

#### Про pthread_join:
Этот пример мы смотрели на лабораторной:
- https://www.ibm.com/docs/en/zos/2.4.0?topic=functions-pthread-join-wait-thread-end

А эти примеры я не успел показать:
- https://www.ibm.com/docs/en/aix/7.2?topic=programming-joining-threads
- https://www.ibm.com/docs/en/aix/7.1?topic=p-pthread-join-pthread-detach-subroutine

Здесь более чётко написано про **блокировку** потока в ожидании завершения указанного потока.

Как я понял, **join** в названии означает **присоединение** дополнительного потока к главному, т.е. главный поток ожидает завершения дополнительного и его присоединения к главному, чтобы дальше выполнялся только главный поток. Такое ожидание присоединения подразумевает блокировку. А для попытки присоединения без блокировки есть функции **pthread_tryjoin_np** и **pthread_timedjoin_np**.

#### Моя реализация
Отмечу, моя реализация является многопоточной, но **не масштабируется**: если создать большое количество клиентов (от 10 и более) или убрать задержки их создания, то начнут появляться ошибки соединения, которые проявляются в получении сервером некорректных данных от клиента. У меня возникала такая ситуация, когда с сервером соединялись несколько клиентов с index=0 и pid=0, что приводило к гонкам и длительному откладыванию потоков.

Как я понял, причиной этого является то, что клиент устанавливает соединение с сервером по протоколу UDP, который не гарантирует доставку пакетов и порядок их получения. В мануале написано, что можно заменить аргумент функции **clnt_create** с "udp" на "tcp" для смены протокола соединения. Но у меня при такой замене клиент выдаёт ошибку

```
call failed: RPC: Server can't decode arguments
```

Решения проблемы я пока не нашёл, но наткнулся на интересный материал от redhat:
https://listman.redhat.com/archives/redhat-list/2004-June/msg00439.html

Также у клиентов установлены таймауты, поэтому они могут не дождаться своей очереди по алгоритму Лампорта "Булочная". Но эта проблема решаема: таймауты можно менять с помощью функции **clnt_control**.