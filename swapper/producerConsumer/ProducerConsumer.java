package producerConsumer;

import java.util.*;
import swapper.Swapper;

public class ProducerConsumer {
    private static Random rng = new Random();

    // ASSERT: (PRODUCERS * PRODUCED % CONSUMERS == 0)
    private static final int PRODUCERS = 2;
    private static final int PRODUCED = 10;
    private static final int CONSUMERS = 5;
    private static final int CONSUMED = (PRODUCERS * PRODUCED / CONSUMERS);
    private static final int BUFFER_SIZE = 10;
    private static final int[] buffer = new int[BUFFER_SIZE];
    private static int producerIdx = 0;
    private static int consumerIdx = 0;
    private static final Swapper<Integer> swapper = new Swapper<>();


    // These work like binary semaphores. We will make sure that there is always only one
    // consumer/producer in the consumers/producers critical section.
    private static final int PRODUCER_AVAILABLE = 0, CONSUMER_AVAILABLE = 1;

    // We use values from 10 to BUFFER_SIZE + 10 to indicate that buffer has some available product
    // at index idx - 10;
    private static int makeProductAvailableNode(int idx) {
        return 10 + idx;
    }

    // We use values from BUFFER_SIZE + 10 to 2 * BUFFER_SIZE + 10 to indicate that buffer has space
    // waiting for producer to make a product at index idx - BUFFER_SIZE - 10.
    private static int makeSpaceAvailableNode(int idx) {
        return 10 + BUFFER_SIZE + idx;
    }

    private static int get() throws InterruptedException {
        swapper.swap(Collections.singleton(CONSUMER_AVAILABLE),
                     Collections.emptySet());

        // Only after we know we are the only consumer, we can wait for the object at index
        // consumeridx to be created (if it has not been already).
        swapper.swap(Collections.singleton(makeProductAvailableNode(consumerIdx)),
                     Collections.emptySet());

        int oldIdx = consumerIdx;
        int retval = buffer[consumerIdx];
        consumerIdx = (consumerIdx + 1) % BUFFER_SIZE;
        System.out.println("\u001B[34mConsumed at index " + oldIdx + "\u001B[0m");

        List<Integer> release = Arrays.asList(new Integer[] {
                CONSUMER_AVAILABLE,
                makeSpaceAvailableNode(oldIdx)
            });
        swapper.swap(Collections.emptySet(), release);

        return retval;
    }

    private static void put(int x) throws InterruptedException {
        swapper.swap(Collections.singleton(PRODUCER_AVAILABLE),
                     Collections.emptySet());

        // Only after we know we are the only producer working, we can wait (if needed). We will
        // wait only if the value at the consumeridx has not been consumed yet.
        swapper.swap(Collections.singleton(makeSpaceAvailableNode(producerIdx)),
                     Collections.emptySet());

        int oldIdx = producerIdx;
        buffer[producerIdx] = x;
        producerIdx = (producerIdx + 1) % BUFFER_SIZE;
        System.out.println("\u001B[35mProduced at index " + oldIdx + "\u001B[0m");

        List<Integer> release = Arrays.asList(new Integer[] {
                PRODUCER_AVAILABLE,
                makeProductAvailableNode(oldIdx)
            });
        swapper.swap(Collections.emptySet(), release);
    }

    private static class Producer implements Runnable {
        private final Random rng;

        public Producer() {
            this.rng = new Random();
        }

        @Override
        public void run() {
            try {
                for (int i = 0; i < PRODUCED; ++i) {
                    put(rng.nextInt(200));
                }
            }
            catch (InterruptedException e) {
                Thread t = Thread.currentThread();
                t.interrupt();
                System.err.println(t.getName() + " interrupted");
            }
        }
    }

    private static class Cunsumer implements Runnable {
        @Override
        public void run() {
            Thread t = Thread.currentThread();
            try {
                int sum = 0;
                int consumed = 0;
                for (int i = 0; i < CONSUMED; ++i) {
                    int x = get();
                    sum += x;
                    ++consumed;
                }
                System.out.println(t.getName() + " Data poritons consumed: "
                                   + consumed + ", sum: " + sum);
            }
            catch (InterruptedException e) {
                t.interrupt();
                System.err.println(t.getName() + " has been interrupted");
            }
        }
    }

    public static void main(String args[]) throws InterruptedException {
        // Initialize the start state of the buffer. Both 'semaphores' for producers and consumers
        // are open and every space in the buffer is available and no product has been created yet.
        ArrayList<Integer> startState = new ArrayList<>();
        startState.add(PRODUCER_AVAILABLE);
        startState.add(CONSUMER_AVAILABLE);
        for(int i = 0; i < BUFFER_SIZE; ++i)
            startState.add(makeSpaceAvailableNode(i));
        swapper.swap(Collections.emptySet(), startState);

        List<Thread> activeThreads = new ArrayList<>();
        for (int i = 0; i < PRODUCERS; ++i) {
            Runnable r = new Producer();
            Thread t = new Thread(r, "Producer" + i);
            activeThreads.add(t);
        }
        for (int i = 0; i < CONSUMERS; ++i) {
            Runnable r = new Cunsumer();
            Thread t = new Thread(r, "Cunsumer" + i);
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
