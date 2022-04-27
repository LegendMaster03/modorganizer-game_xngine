public class MenuFile {
    private List<String> lines;

    public MenuFile() {
        lines = new ArrayList<>();
    }

    public MenuFile(MenuFile other) {
        this();
        lines.addAll(other.lines);
    }

    public void readFile(File fileToRead) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(fileToRead));
        for (String line = reader.readLine(); line != null; line = reader.readLine()) {
            lines.add(line);
        }
        reader.close();
    }

    public void writeFile(File fileToWrite) throws IOException {
        BufferedWriter writer = new BufferedWriter(new FileWriter(fileToWrite));
        for (String line : lines) {
            writer.write(line);
            writer.newLine();
        }
        writer.close();
    }

    public void readChanges(File changesFile) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(changesFile));
        Map<String, String> lineChanges = new HashMap<>();

        for (String line = reader.readLine(); line != null; line = reader.readLine()) {
            String[] split = line.split("=");
            if (split.length == 2) {
                lineChanges.put(split[0], split[1]);
            }
        }
        reader.close();

        for (int i = 0; i < lines.size(); i++) {
            String[] split = lines.get(i).split("=");
            if (split.length == 2) {
                if (lineChanges.containsKey(split[0])) {
                    lines.set(i, lineChanges.get(split[0]));
                }
            }
        }
    }
}