import java.util.Arrays;

class Scope0 {

  public int N;
  public int[] row;
  public int[] col;
  public int[] diag1;
  public int[] diag2;

  Scope0() {
    N = 8;
    row = new int[N];
    Arrays.fill(row, 0);
    col = new int[N];
    Arrays.fill(col, 0);
    diag1 = new int[N + N - 1];
    Arrays.fill(diag1, 0);
    diag2 = new int[N + N - 1];
    Arrays.fill(diag2, 0);
  }
}

class Queens4 {

  public static void main(String[] args) {
    _try(new Scope0(), 0);
  }

  static void printboard(Scope0 _scope) {
    for (int i = 0; i <= _scope.N - 1; i++) {
      for (int j = 0; j <= _scope.N - 1; j++) {
        System.out.print(_scope.col[i] == j ? " O" : " .");
      }
      System.out.print("\n");
    }
    System.out.print("\n");
  }

  static void _try(Scope0 _scope, int c) {
    if (c == _scope.N) {
      printboard(_scope);
    } else {
      for (int r = 0; r <= _scope.N - 1; r++) {
        if (_scope.row[r] == 0 && _scope.diag1[r + c] == 0 && _scope.diag2[r + 7 - c] == 0) {
          _scope.row[r] = 1;
          _scope.diag1[r + c] = 1;
          _scope.diag2[r + 7 - c] = 1;
          _scope.col[c] = r;
          _try(_scope, c + 1);
          _scope.row[r] = 0;
          _scope.diag1[r + c] = 0;
          _scope.diag2[r + 7 - c] = 0;
        }
      }
    }
  }
}
