import java.util.Arrays;

class Queens2 {

  static void print(String s) {
    System.out.print(s);
  }

  static void printi(int i) {
    System.out.print(i);
  }

  static <T> T[] fill(T[] a, T v) {
    Arrays.fill(a, v);
    return a;
  }

  static int[] fill(int[] a, int v) {
    Arrays.fill(a, v);
    return a;
  }

  static String[] fill(String[] a, String v) {
    Arrays.fill(a, v);
    return a;
  }

  class __Let1__ {
    int N = 8;
    int[] row = fill(new int[N], 0);
    int[] col = new int[N];
    int[] diag1 = new int[N + N - 1];
    int[] diag2 = new int[N + N - 1];

    __Let1__() {
      Arrays.fill(row, 0);
      Arrays.fill(col, 0);
      Arrays.fill(diag1, 0);
      Arrays.fill(diag2, 0);
    }

    void printboard() {
      for (int i = 0; i <= N - 1; i++) {
        for (int j = 0; j <= N - 1; j++) {
          print(col[i] == j ? " O" : " .");
        }
        print("\n");
      }
      print("\n");
    }

    void __try__(int c) {
      if (c == N) {
        printboard();
      } else {
        for (int r = 0; r <= N - 1; r++) {
          if (row[r] == 0 && diag1[r + c] == 0 && diag2[r + 7 - c] == 0) {
            row[r] = 1;
            diag1[r + c] = 1;
            diag2[r + 7 - c] = 1;
            col[c] = r;
            __try__(c + 1);
            row[r] = 0;
            diag1[r + c] = 0;
            diag2[r + 7 - c] = 0;
          }
        }
      }
    }

    void run() {
      __try__(0);
    }
  }

  void run() {
    new __Let1__().run();
  }

  public static void main(String[] args) {
    new Queens2().run();
  }
}
