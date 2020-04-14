$(OBJ_DIR)/%.yu.ll: $(TOP_DIR)/%.yu
	echo "YUC $@"
	-mkdir -p $(dir $@)
	$(YUC) $(YUCFLAGS) -ot llvm $^ > $@

$(OBJ_DIR)/%.c.o: $(TOP_DIR)/%.c
	echo "CC  $@"
	-mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(OBJ_DIR)/%.ll
	echo "LLC $@"
	$(LLC) $(LLCFLAGS) $^ -o $@

$(OBJ_DIR)/%.S.o: $(TOP_DIR)/%.S
	echo "AS  $@"
	-mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^
