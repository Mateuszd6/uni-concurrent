package swapper;

import java.util.*;
import java.util.concurrent.*;

public class Swapper<E> {
    private class Foo<E> {
        private Collection<E> removed;
        private Collection<E> added;
        private Semaphore sem;
        private boolean dead;

        public Foo(Collection<E> removed, Collection<E> added) {
            this.removed = removed;
            this.added = added;
            this.sem = new Semaphore(0);
            this.dead = false;
        }
    }

    private ArrayList<Foo<E>> awaiting;
    private HashSet<E> set;
    private Semaphore mutex;

    public Swapper() {
        set = new HashSet<>();
        awaiting = new ArrayList<>();
        mutex = new Semaphore(1);
    }

    // Returns Foo with the next thread that can perform the swap,
    // or null, if no one from awaiting threads can do it.
    private Foo<E> getNextAwaitingThreadIfPossible() {
        for(int i = 0; i < awaiting.size(); ++i) {
            if(awaiting.get(i).dead) {
                ///
                System.out.print(String.format(
                                     "Thread %s has removed an obsolating semaphore removing %s\n",
                                     Thread.currentThread().getName(),
                                     awaiting.get(i).removed));
                ///

                awaiting.remove(i);
                --i;
                continue;
            }

            if(set.containsAll(awaiting.get(i).removed)) {
                Foo<E> f = awaiting.remove(i);
                return f;
            }
        }

        return null;
    }

    public void swap(Collection<E> removed, Collection<E> added)
        throws InterruptedException {

        ///
        System.out.print(String.format(
                             "Thread %s wants to swap: %s ---> %s\n",
                             Thread.currentThread().getName(),
                             removed, added));
        ///

        mutex.acquire();

        if(!set.containsAll(removed)) {

            ///
            System.out.print(String.format(
                                 "Thread %s cannot swap and waits...\n",
                                 Thread.currentThread().getName()));
            ///

            Foo<E> foo = new Foo<>(removed, added);
            awaiting.add(foo);

            mutex.release();

            try {
                foo.sem.acquire();
            }
            catch (InterruptedException e){
                ///
                System.out.print(String.format(
                                     "Thread %s has been interrupted!\n",
                                     Thread.currentThread().getName()));
                ///

                foo.dead = true;
                Thread.currentThread().interrupt();
                throw new InterruptedException();
            }

            ///
            System.out.print(String.format(
                                 "Thread %s wakes!\n",
                                 Thread.currentThread().getName()));
            ///
        }

        set.removeAll(removed);
        set.addAll(added);

        ///
        System.out.print(String.format(
                             "Thread %s _SWAPS_ %s ---> %s\n\tSwapper now: %s\n",
                             Thread.currentThread().getName(),
                             removed, added, set));
        ///

        Foo<E> nextSwapData = getNextAwaitingThreadIfPossible();
        if(nextSwapData != null)
        {
            // Next swapping thread starts here, as we finish.
            nextSwapData.sem.release();
        }
        else
            mutex.release();
    }
}
