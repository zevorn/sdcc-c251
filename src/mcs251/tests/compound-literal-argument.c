typedef struct
{
  long long ticks;
} timeout_t;

typedef struct
{
  int key;
} lock_key_t;

struct lock
{
  int state;
};

extern int wait_for_event (struct lock *, lock_key_t, void *, timeout_t);

#define WAIT_FOREVER ((timeout_t) { (long long) -1 })

int
wait_forever (struct lock *lock, lock_key_t key)
{
  int result = wait_for_event (lock, key, 0, WAIT_FOREVER);

  return result;
}
