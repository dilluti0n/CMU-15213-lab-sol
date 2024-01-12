# Lab0 CProgrammingLab

The original tar file is `cprogramminglab-handout.tar` and the writeup is
`cprogramminglab.pdf`. My solution is in `cprogramminglab-handout`.

After untaring cprogramminglab-handout.tar, you follow the instructions in 
`cprogramming.pdf` to untar the folder. You can run `make` directly, but you
cannot run `make test`, since `./driver.py` is used. 

- To run `make test`:
1. Install python2 in your environment;
2. Change `./driver.py` in `Makefile` to `python2 ./driver.py`;
3. Run `make test`;

- To use `driver.py` directly:
1. Install python2 in your environment;
2. Run `python2 driver.py`.

I did this lab assignment before self-studying 15-213, and it was difficult and time consuming, so I studied K&R C first. 
My solutions to its exercises can be found at [sols](https://github.com/dilluti0n/sols), and the commentary in Section 6, which is particularly helpful in solving this one, can be found at [readme_chapter_6.md](https://github.com/dilluti0n/sols/blob/master/6/readme_chapter_6.md).
It's written in Korean, but it doesn't use complex sentences, so you should be able to read it with Google Translater.

The function `q_free` in my code tests whether value is `NULL` before `free` the value of each list element, which is unnecessary in this code because if the malloc of value of `q_insert_head` or `q_insert_tail` fails, the function exits without creating a node, and there is no way to create a node without giving it a value.
A personal difficulty in solving this problem was that I had to implement `free` and `add` without using recursive functions, 
unlike what the authors of K&R C did. This was achieved by using variables such as `nxt` and `prv`.
