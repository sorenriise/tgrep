tgrep
=====

Simple text index and search program.

In case where you have a large corpus of test which you frequently grep you can save time by pre-calculate a simple index of the files.

While there is a lot of indexing tools out there, elastic search as one, they are more convoluted to use and does not easily follow the Unix Command Line paradime.   This tools allow you to write bash script using other command line tools to achive your gools


Limitations;
  * The index runs once -- if the files have changing context, then this may not work for you

Install;
  * make; make install


Usage;

  * Create index;  (will streamline this later)

      tgrep --index [list of files with text]
      sort -n -o /var/tgrep/spool /var/tgrep/spool
      tgrep --unspool

  * Search

      tgrep 'Mr Jones' '@yahoo'
      this will result in a list of files likely to contain the strings.


