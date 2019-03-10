package readerWriter;

import java.util.*;
import swapper.Swapper;

public class ReaderWriter {
    private static final Random rng = new Random();
    private static final Swapper<Integer> swapper = new Swapper<>();
    private static final int WRITERS = 4;
    private static final int READERS = 20;

    private static int readersReading = 0;
    private static int writersWaiting = 0;

    private static final Integer MUTEX_AVAILABLE = 0,
        READERS_DO_NOT_READ = 1,
        WRITERS_DO_NOT_WAIT = 3,
        OPENED = 4;

    private static void read() throws InterruptedException {
        System.out.println("\u001B[34m" + Thread.currentThread().getName() + " reads.\u001B[0m");
        Thread.sleep(rng.nextInt(4000));
        System.out.println("\u001B[34m" + Thread.currentThread().getName() + " ends reading.\u001B[0m");
    }

    private static void write() throws InterruptedException {
        System.out.println("\u001B[35m" + Thread.currentThread().getName() + " writes.\u001B[0m");
        Thread.sleep(rng.nextInt(100));
        System.out.println("\u001B[35m" + Thread.currentThread().getName() + " ends writing.\u001B[0m");

    }

    private static void queueAndRead() throws InterruptedException {
        List<Integer> list = Arrays.asList(new Integer[]{ OPENED, WRITERS_DO_NOT_WAIT });
        swapper.swap(list, list);

        // After it gets, just before reading:
        swapper.swap(Collections.singleton(MUTEX_AVAILABLE), Collections.emptySet());
        if(readersReading == 0)
            swapper.swap(Collections.singleton(READERS_DO_NOT_READ), Collections.emptySet());
        readersReading++;
        swapper.swap(Collections.emptySet(), Collections.singleton(MUTEX_AVAILABLE));

        read();

        swapper.swap(Collections.singleton(MUTEX_AVAILABLE), Collections.emptySet());
        readersReading--;
        if(readersReading == 0) {
            System.out.println("\u001B[31m Last reader exits!\u001B[0m");
            swapper.swap(Collections.emptySet(), Collections.singleton(READERS_DO_NOT_READ));
        }
        swapper.swap(Collections.emptySet(), Collections.singleton(MUTEX_AVAILABLE));
    }

    private static void queueAndWrite() throws InterruptedException {
        swapper.swap(Collections.singleton(MUTEX_AVAILABLE), Collections.emptySet());
        if(writersWaiting == 0)
            swapper.swap(Collections.singleton(WRITERS_DO_NOT_WAIT), Collections.emptySet());
        writersWaiting++;
        swapper.swap(Collections.emptySet(), Collections.singleton(MUTEX_AVAILABLE));

        List<Integer> list = Arrays.asList(new Integer[] { OPENED, READERS_DO_NOT_READ });
        swapper.swap(list, Collections.singleton(READERS_DO_NOT_READ));
        write();

        swapper.swap(Collections.singleton(MUTEX_AVAILABLE), Collections.emptySet());
        writersWaiting--;
        if(writersWaiting == 0) {
            swapper.swap(Collections.emptySet(), Collections.singleton(WRITERS_DO_NOT_WAIT));
        }
        swapper.swap(Collections.emptySet(), Collections.singleton(MUTEX_AVAILABLE));

        swapper.swap(Collections.emptySet(), Collections.singleton(OPENED));
    }

    private static class Writer implements Runnable {

        @Override
        public void run() {
            Thread t = Thread.currentThread();
            try {
                Thread.sleep(rng.nextInt(1000));
                System.out.println(t.getName() + " queues up for writing.");
                queueAndWrite();
                System.out.println(t.getName() + " has finished and exists.");
            }
            catch (InterruptedException e) {
                t.interrupt();
                System.err.println(t.getName() + " interrupted");
            }
        }
    }

    private static class Reader implements Runnable {

        @Override
        public void run() {
            Thread t = Thread.currentThread();
            try {
                Thread.sleep(rng.nextInt(1000));
                System.out.println(t.getName() + " queues up for reading.");
                queueAndRead();
                System.out.println(t.getName() + " has finished and exists.");
            }
            catch (InterruptedException e) {
                t.interrupt();
                System.err.println(t.getName() + " has been interrupted");
            }
        }
    }

    public static void main(String[] args) throws InterruptedException {
        List<Integer> startState = Arrays.asList(new Integer[] {
                MUTEX_AVAILABLE,
                READERS_DO_NOT_READ,
                WRITERS_DO_NOT_WAIT,
                OPENED
            });
        swapper.swap(Collections.emptySet(), startState);

        List<Thread> activeThreads = new ArrayList<>();
        for (int i = 0; i < READERS; ++i) {
            Runnable r = new Reader();
            Thread t = new Thread(r, "Reader" + i);
            activeThreads.add(t);
        }
        for (int i = 0; i < WRITERS; ++i) {
            Runnable r = new Writer();
            Thread t = new Thread(r, "Writer" + i);
            activeThreads.add(t);
        }

        for (Thread t : activeThreads) {
            t.start();
        }

        try {
            for (Thread t : activeThreads) {
                t.join();
            }
        }
        catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            System.err.println("Main thread has been interrupted!");
        }
    }
}
