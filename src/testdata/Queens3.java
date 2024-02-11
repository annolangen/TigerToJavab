import java.util.Arrays;

class Queens3 {
  static void printboard(int N, int[] col) {
    for (int i = 0; i <= N - 1; i++) {
      for (int j = 0; j <= N - 1; j++) {
        System.out.print(col[i] == j ? " O" : " .");
      }
      System.out.print("\n");
    }
    System.out.print("\n");
  }

  static void _try(int c, int N, int[] row, int[] diag1, int[] diag2, int[] col) {
    if (c == N) {
      printboard(N, col);
    } else {
      for (int r = 0; r <= N - 1; r++) {
        if (row[r] == 0 && diag1[r + c] == 0 && diag2[r + 7 - c] == 0) {
          row[r] = 1;
          diag1[r + c] = 1;
          diag2[r + 7 - c] = 1;
          col[c] = r;
          _try(c + 1, N, row, diag1, diag2, col);
          row[r] = 0;
          diag1[r + c] = 0;
          diag2[r + 7 - c] = 0;
        }
      }
    }
  }

  public static void main(String[] args) {
    int N = 8;
    int[] row = new int[N];
    Arrays.fill(row, 0);
    int[] col = new int[N];
    Arrays.fill(col, 0);
    int[] diag1 = new int[N + N - 1];
    Arrays.fill(diag1, 0);
    int[] diag2 = new int[N + N - 1];
    Arrays.fill(diag2, 0);
    _try(0, N, row, diag1, diag2, col);
  }
}
