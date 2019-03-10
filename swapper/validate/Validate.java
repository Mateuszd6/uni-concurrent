package validate;

import java.util.*;
import java.util.Collection;
import java.util.Collections;

import swapper.Swapper;

public class Validate {

    private static class DelaySwap<E> implements Runnable
    {
        public int delay;
        public Collection<E> remove;
        public Collection<E> add;
        public Swapper<E> swapperRef;

        public DelaySwap(int delay,
                         Collection<E> remove,
                         Collection<E> add,
                         Swapper<E> swapperRef) {
            this.delay = delay;
            this.remove = remove;
            this.add = add;
            this.swapperRef = swapperRef;
        }

        @Override
        public void run() {
            try {
                Thread.sleep(delay);
                swapperRef.swap(remove, add);
            }
            catch (InterruptedException e) {
            }
        }
    }

    public static void main(String[] args) {
        Swapper<Integer> swapper = new Swapper<>();
        ArrayList<Thread> workingThreads = new ArrayList<>();

        {
            HashSet<Integer> from = new HashSet<>();
            from.add(3);
            from.add(4);
            HashSet<Integer> to = new HashSet<>();
            to.add(2);
            to.add(2);
            to.add(2);
            to.add(2);
            to.add(9);

            DelaySwap<Integer> test = new DelaySwap<>(1000, from, to, swapper);
            Thread t = new Thread(test, "1");
            workingThreads.add(t);
            t.start();
        }

        {
            HashSet<Integer> from = new HashSet<>();
            from.add(1);
            HashSet<Integer> to = new HashSet<>();
            to.add(2);
            to.add(3);
            to.add(4);
            to.add(5);
            DelaySwap<Integer> test = new DelaySwap<>(2000, from, to, swapper);
            Thread t = new Thread(test, "2");
            workingThreads.add(t);
            t.start();
        }

        {
            HashSet<Integer> from = new HashSet<>();
            from.add(2);
            from.add(5);
            HashSet<Integer> to = new HashSet<>();
            DelaySwap<Integer> test = new DelaySwap<>(0, from, to, swapper);
            Thread t = new Thread(test, "3");
            workingThreads.add(t);
            t.start();
        }


        {
            HashSet<Integer> from = new HashSet<>();
            from.add(2);
            from.add(9);
            HashSet<Integer> to = new HashSet<>();
            DelaySwap<Integer> test = new DelaySwap<>(0, from, to, swapper);
            Thread t = new Thread(test, "4");
            workingThreads.add(t);
            t.start();

            try {
                Thread.sleep(500);
            }
            catch (Exception e){
                ;
            }

            t.interrupt();
        }

        try {
            Collection<Integer> empty = Collections.emptySet();
            Collection<Integer> singletonOne = Collections.singleton(1);
            Collection<Integer> singletonTwo = Collections.singleton(2);
            swapper.swap(empty, Collections.singleton(1));
            // swapper.swap(Collections.singleton(1), empty);

            for(Thread t : workingThreads)
                t.join();

            System.out.println("OK");
        }
        catch (InterruptedException e) {
            System.out.println("ERROR");
            System.exit(1);
        }
    }
}
