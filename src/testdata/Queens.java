import java.util.Arrays;

class Queens {

  interface FT0 {
    void run();
  }

  interface FT1 {
    void run(int x, FT1 next);
  }

  public static void main(String[] args) {
    int N = 8;
    int[] row = new int[8];
    Arrays.fill(row, 0);
    int[] col = new int[8];
    Arrays.fill(col, 0);
    int[] diag1 = new int[N + N - 1];
    Arrays.fill(diag1, 0);
    int[] diag2 = new int[N + N - 1];
    Arrays.fill(diag2, 0);

    FT0 printboard = () -> {
      for (int i = 0; i <= N - 1; i++) {
        for (int j = 0; j <= N - 1; j++) {
          System.out.print(col[i] == j ? " O" : " .");
        }
        System.out.print("\n");
      }
      System.out.print("\n");
    };

    FT1 __try__ = (int c, FT1 __rec__) -> {
      if (c == N) {
        printboard.run();
      } else {
        for (int r = 0; r <= N - 1; r++) {
          if (row[r] == 0 && diag1[r + c] == 0 && diag2[r + 7 - c] == 0) {
            row[r] = 1;
            diag1[r + c] = 1;
            diag2[r + 7 - c] = 1;
            col[c] = r;
            __rec__.run(c + 1, __rec__);
            row[r] = 0;
            diag1[r + c] = 0;
            diag2[r + 7 - c] = 0;
          }
        }
      }
    };

    __try__.run(0, __try__);
  }
}
