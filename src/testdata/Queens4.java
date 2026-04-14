import java.util.Arrays;

class Scope0 {

  static int[] MakeIntArray(int n, int v) {
    int[] result = new int[n];
    Arrays.fill(result, v);
    return result;
  }

  public int N = 8;
  public int[] row = MakeIntArray(N, 0);
  public int[] col = MakeIntArray(N, 0);
  public int[] diag1 = MakeIntArray(N + N - 1, 0);
  public int[] diag2 = MakeIntArray(N + N - 1, 0);
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
