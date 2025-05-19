with Ada.Text_IO;
with Ada.Command_Line;
with Ada.Unchecked_Deallocation;
with Ada.Strings.Fixed;
with Interfaces.C; -- For char type for byte arrays
with System;
with System.Address_To_Access_Conversions;
with Ada.Unchecked_Conversion;

procedure Ada_Error_Generator is
   package TIO renames Ada.Text_IO;

   -- Helper for printing messages
   procedure Print_Action(Message : String) is
   begin
      TIO.Put_Line("--- Triggering " & Message & " ---");
   end Print_Action;

   -- Types for leaks and memory errors
   type Data_Record is record
      ID   : Integer;
      Name : String(1 .. 20);
   end record;
   type Data_Record_Ptr is access Data_Record;

   procedure Free_Data_Record is new Ada.Unchecked_Deallocation(Data_Record, Data_Record_Ptr);

   type Byte_Block is array (Positive range <>) of Interfaces.C.char;
   type Byte_Block_Ptr is access Byte_Block;
   procedure Free_Byte_Block is new Ada.Unchecked_Deallocation(Byte_Block, Byte_Block_Ptr);

   -- For indirect leaks
   type Node;
   type Node_Ptr is access Node;
   type Node is record
      Data : Integer;
      Next : Node_Ptr;
   end record;
   procedure Free_Node is new Ada.Unchecked_Deallocation(Node, Node_Ptr);


   -- 1. Definite Leak
   procedure Cause_Definite_Leak is
      Leaked_Ptr : Data_Record_Ptr;
   begin
      Print_Action("Definite Leak");
      Leaked_Ptr := new Data_Record'(ID => 1, Name => "Definitely Leaked   ");
      TIO.Put_Line("Allocated for definite leak: ID = " & Integer'Image(Leaked_Ptr.ID));
      -- Leaked_Ptr goes out of scope without deallocation, or we can null it:
      -- Leaked_Ptr := null;
   end Cause_Definite_Leak;

   -- 2. Indirect Leak
   function Create_Child_Node return Node_Ptr is
      Child : Node_Ptr;
   begin
      Child := new Node'(Data => 200, Next => null);
      TIO.Put_Line("Allocated child node for indirect leak.");
      return Child;
   end Create_Child_Node;

   procedure Cause_Indirect_Leak is
      Parent_Ptr : Node_Ptr;
   begin
      Print_Action("Indirect Leak");
      Parent_Ptr := new Node'(Data => 100, Next => Create_Child_Node);
      TIO.Put_Line("Allocated parent node for indirect leak.");
      -- Parent_Ptr goes out of scope, leaking parent (definite) and child (indirect)
      -- Parent_Ptr := null;
   end Cause_Indirect_Leak;

   -- 3. Still Reachable Leak
   Global_Reachable_Ptr : Data_Record_Ptr := null;

   procedure Cause_Still_Reachable_Leak is
   begin
      Print_Action("Still Reachable Leak (Global)");
      Global_Reachable_Ptr := new Data_Record'(ID => 2, Name => "Still Reachable   ");
      TIO.Put_Line("Setup global still reachable pointer: ID = " & Integer'Image(Global_Reachable_Ptr.ID));
      -- Global_Reachable_Ptr is not freed
   end Cause_Still_Reachable_Leak;

   -- 4. Invalid Write (Heap)
   procedure Cause_Invalid_Write_Heap is
      Buffer : Byte_Block_Ptr;
   begin
      Print_Action("Invalid Write (Heap)");
      Buffer := new Byte_Block'(1 .. 10 => Interfaces.C.nul);
      TIO.Put_Line("About to cause an invalid heap write...");
      begin
         Buffer(15) := Interfaces.C.To_C('X'); -- Invalid write past allocated heap region
         TIO.Put_Line("Invalid heap write attempted (Buffer(15)).");
      exception
         when Constraint_Error =>
            TIO.Put_Line("Caught Constraint_Error during invalid heap write. Valgrind might still see underlying issue.");
      end;
      Free_Byte_Block(Buffer);
   end Cause_Invalid_Write_Heap;

   -- 5. Invalid Read (Heap)
   procedure Cause_Invalid_Read_Heap is
      Buffer : Byte_Block_Ptr;
      C      : Interfaces.C.char := Interfaces.C.nul;
   begin
      Print_Action("Invalid Read (Heap)");
      Buffer := new Byte_Block'(1 .. 10 => Interfaces.C.To_C('A'));
      TIO.Put_Line("About to cause an invalid heap read...");
      begin
         C := Buffer(20); -- Invalid read past allocated heap region
         TIO.Put_Line("Invalid heap read attempted (value: " & Interfaces.C.To_Ada(C) & ")");
      exception
         when Constraint_Error =>
            TIO.Put_Line("Caught Constraint_Error during invalid heap read. Valgrind might still see underlying issue.");
      end;
      Free_Byte_Block(Buffer);
   end Cause_Invalid_Read_Heap;

   -- 6. Use of Uninitialized Value (Conditional)
   procedure Cause_Use_Uninitialized_Value is
      Uninitialized_Var : Integer; -- Not initialized
      Result            : Integer := 5;
   begin
      Print_Action("Use of Uninitialized Value in Conditional");
      TIO.Put_Line("About to use an uninitialized value in a conditional...");
      if Uninitialized_Var > 10 then -- Conditional jump depends on uninitialized value
         Result := 10;
      end if;
      TIO.Put_Line("Result after uninitialized check:" & Integer'Image(Result));
   end Cause_Use_Uninitialized_Value;

   -- 7. Double Free
   procedure Cause_Double_Free is
      Ptr : Data_Record_Ptr;
   begin
      Print_Action("Double Free");
      Ptr := new Data_Record'(ID => 3, Name => "Double Free Target  ");
      TIO.Put_Line("About to cause a double free...");
      Free_Data_Record(Ptr);
      TIO.Put_Line("First free done.");
      begin
         Free_Data_Record(Ptr); -- Double free
         TIO.Put_Line("Second free attempted (unexpected for Valgrind error if runtime doesn't catch).");
      exception
         when others => -- Behavior of freeing already freed pointer is undefined.
            TIO.Put_Line("Exception caught attempting second free. Valgrind should report it.");
      end;
   end Cause_Double_Free;

   -- 8. Invalid Free (e.g., freeing stack memory)
   procedure Cause_Invalid_Free_Stack is
      Stack_Var : aliased Integer := 123; -- aliased to take 'Address
      package Addr_Conv is new System.Address_To_Access_Conversions (Integer);
      Stack_Ptr : Addr_Conv.Object_Pointer;
      procedure Free_Int_Stack is new Ada.Unchecked_Deallocation(Integer, Addr_Conv.Object_Pointer);
   begin
      Print_Action("Invalid Free (Stack Address)");
      Stack_Ptr := Addr_Conv.To_Pointer(Stack_Var'Address);
      TIO.Put_Line("About to cause an invalid free (stack address)...");
      begin
         Free_Int_Stack(Stack_Ptr);
         TIO.Put_Line("Invalid free of stack address attempted.");
      exception
         when others =>
            TIO.Put_Line("Exception caught attempting to free stack address. Valgrind should report it.");
      end;
   end Cause_Invalid_Free_Stack;

   -- Invalid Free (mid-block)
   procedure Cause_Invalid_Free_Mid_Block is
      Buffer   : Byte_Block_Ptr;
      Mid_Addr : System.Address;
      Bad_Ptr  : Byte_Block_Ptr;
      function To_Byte_Block_Ptr is new Ada.Unchecked_Conversion(System.Address, Byte_Block_Ptr);
   begin
      Print_Action("Invalid Free (Mid-Block)");
      Buffer := new Byte_Block'(1 .. 30 => Interfaces.C.To_C('B'));
      if Buffer = null or else Buffer.all'Length < 10 then
         TIO.Put_Line("Failed to allocate buffer for mid-block free test.");
         return;
      end if;

      Mid_Addr := Buffer(5)'Address; -- Address of the 5th element (0-indexed if from C, 1-indexed in Ada)
      Bad_Ptr  := To_Byte_Block_Ptr(Mid_Addr);

      TIO.Put_Line("About to cause an invalid free (mid-block)...");
      begin
         Free_Byte_Block(Bad_Ptr);
         TIO.Put_Line("Mid-block free attempted.");
      exception
         when others =>
            TIO.Put_Line("Exception caught attempting mid-block free. Valgrind should report it.");
      end;
      -- If Bad_Ptr free failed, Buffer might still be valid. If it "succeeded", heap is corrupt.
      -- For safety in test, try to free original if not null, but expect issues.
      if Buffer /= null then
         begin
            Free_Byte_Block(Buffer);
            TIO.Put_Line("Original buffer freed after mid-block attempt (if it was still valid).");
         exception
            when others =>
               TIO.Put_Line("Exception freeing original buffer after mid-block attempt; heap likely corrupted.");
         end;
      end if;
   end Cause_Invalid_Free_Mid_Block;

   -- 9. Source and Destination Overlap (using Ada.Strings.Fixed.Move)
   procedure Cause_Overlap_Move is
      Buffer_Size : constant Positive := 30;
      Buffer      : String(1 .. Buffer_Size);
   begin
      Print_Action("Source/Destination Overlap (Ada.Strings.Fixed.Move)");
      Buffer := "1234567890abcdefghij" & (1 .. Buffer_Size - 20 => ' ');
      TIO.Put_Line("Buffer before overlap: " & Buffer(1..20));
      TIO.Put_Line("About to cause overlap with Move (src 1..10, target 6..15)...");
      begin
         -- Target (6..15) overlaps Source (1..10) because elements 6-10 of source
         -- are part of the target destination.
         Ada.Strings.Fixed.Move(Source => Buffer(1 .. 10), Target => Buffer(6 .. 15));
         TIO.Put_Line("Buffer after overlap:  " & Buffer(1..20));
      exception
         when Constraint_Error =>
             TIO.Put_Line("Caught Constraint_Error during Move. Valgrind might also report.");
      end;
   end Cause_Overlap_Move;

   -- 10. Invalid Write (Stack Buffer Overflow)
   procedure Cause_Invalid_Write_Stack is
      Stack_Buffer : array (1 .. 10) of Character;
   begin
      Print_Action("Invalid Write (Stack Buffer Overflow)");
      TIO.Put_Line("About to cause stack buffer overflow...");
      begin
         for I in 1 .. 15 loop -- Write past the end
            Stack_Buffer(I) := Character'Val(Character'Pos('A') + (I mod 10));
         end loop;
         TIO.Put_Line("Stack buffer overflow attempted.");
      exception
         when Constraint_Error =>
            TIO.Put_Line("Caught Constraint_Error during stack write. Valgrind might still see underlying issue.");
      end;
      -- Dummy read to potentially trigger Valgrind if memory was corrupted
      if Stack_Buffer(1) = 'A' then
          TIO.Put_Line("Stack_Buffer(1) check after attempted overflow.");
      end if;
   end Cause_Invalid_Write_Stack;

   -- 11. Invalid Read (Stack Buffer Overflow/Underflow)
   procedure Cause_Invalid_Read_Stack is
      Stack_Buffer : array (1 .. 10) of Character := (others => 'Z');
      C            : Character := ' ';
   begin
      Print_Action("Invalid Read (Stack Buffer Overflow)");
      TIO.Put_Line("About to cause stack buffer overflow read...");
      begin
         C := Stack_Buffer(15); -- Overflow read
         TIO.Put_Line("Stack buffer overflow read attempted, value: " & C);
      exception
         when Constraint_Error =>
            TIO.Put_Line("Caught Constraint_Error during stack read. Valgrind might still see underlying issue.");
      end;
   end Cause_Invalid_Read_Stack;

begin
   TIO.Put_Line("Ada Valgrind Test Error Generator Starting...");
   TIO.Put_Line("This program generates various errors and leaks for Valgrind to parse.");
   TIO.Put_Line("NOTE: Some errors may terminate the program early.");
   TIO.Put_Line("Valgrind should report errors that occurred before termination.");
   TIO.New_Line;

   -- Leaks (generally non-fatal for program execution)
   Cause_Definite_Leak;          TIO.New_Line;
   Cause_Indirect_Leak;          TIO.New_Line;
   Cause_Still_Reachable_Leak;   TIO.New_Line;

   -- Potentially non-fatal errors
   Cause_Use_Uninitialized_Value; TIO.New_Line;
   Cause_Overlap_Move;            TIO.New_Line;

   -- Stack-based memory errors
   Cause_Invalid_Write_Stack;     TIO.New_Line;
   Cause_Invalid_Read_Stack;      TIO.New_Line;

   -- Heap memory errors (more likely to be fatal or destabilizing)
   Cause_Invalid_Write_Heap;      TIO.New_Line;
   Cause_Invalid_Read_Heap;       TIO.New_Line;

   -- Invalid free operations (highly likely to be fatal or cause issues)
   -- Order these carefully as they can corrupt the heap significantly.
   Cause_Invalid_Free_Stack;      TIO.New_Line;
   Cause_Invalid_Free_Mid_Block;  TIO.New_Line;

   -- Double free is often very fatal and should be one of the last heap operations.
   Cause_Double_Free;             TIO.New_Line;

   TIO.Put_Line("Ada Valgrind Test Error Generator Finishing (if not crashed).");
   TIO.Put_Line("Expected leaks to be reported by Valgrind:");
   TIO.Put_Line("- Definite leak from Cause_Definite_Leak");
   TIO.Put_Line("- Indirect leak from Cause_Indirect_Leak (parent is definite, child is indirect)");
   TIO.Put_Line("- Still reachable leak from Global_Reachable_Ptr");
end Ada_Error_Generator;