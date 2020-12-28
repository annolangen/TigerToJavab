public class Std {
  public static void print(String s) { System.out.print(s); }
  public static void printi(int i) { System.out.print(Integer.toString(i)); }
  public static void flush() { System.out.flush(); }
  public static String getChar() {
    try {
      int c = System.in.read();
      if (c >= 0) return chr(c);
    } catch (Exception ignored) {
    }
    return "";
  }
  public static int ord(String s) { return s.length() > 0 ? s.charAt(0) : -1; }
  public static String chr(int i) { return new String(new char[] {(char)i}); }
  public static int size(String s) { return s.length(); }
  public static String substring(String s, int f, int n) {
    return s.substring(f, f + n);
  }
  public static String concat(String s, String t) { return s + t; }
  public static int not(int i) { return i == 0 ? 1 : 0; }
  public static void exit(int i) { System.exit(i); }
}
