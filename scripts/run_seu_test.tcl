#tcl script for ODMB radiation test, to be controlled with Windows GUI

#run with vivado -nojournal -nolog -mode batch -notrace -source run_seu_test.tcl
#can test non-vivado commands with !/usr/bin/tclsh

#settings for Higgs
set output_file_name "comm_files/tcl_comm_out.txt"
set input_file_name "comm_files/tcl_comm_in.txt"

#settings for Higgsino
#set output_file_name "comm_files\\tcl_comm_out.txt"
#set input_file_name "comm_files\\tcl_comm_in.txt"

proc parse_report {objname propertyname index} {
  # =================================================================
  # Format of the return info consist as follows, this function returns only the value
  # ----------------------------------------------------------------
  # Property              Type    Read-only  Value
  # RX_BER                string  true       1.2844469155632799e-13
  # RX_RECEIVED_BIT_COUNT string  true       113905209031560
  # RX_PATTERN            enum    false      PRBS 7-bit
  # =================================================================
  
  set fullrep [split [report_property $objname $propertyname -return_string] "\n"]
  set valline [lindex $fullrep 1]
  set result [lindex [regexp -all -inline {\S+} $valline] $index]
  return $result 
}

proc reset_links_all {links} {
  # Reset error count
  set_property LOGIC.MGT_ERRCNT_RESET_CTRL 1 [get_hw_sio_links $links]
  commit_hw_sio [get_hw_sio_links $links]
  set_property LOGIC.MGT_ERRCNT_RESET_CTRL 0 [get_hw_sio_links $links]
  commit_hw_sio [get_hw_sio_links $links]
  
  ## Inject 1 error
  #set_property LOGIC.ERR_INJECT_CTRL 1 [get_hw_sio_links $links]
  #commit_hw_sio [get_hw_sio_links $links]
  #set_property LOGIC.ERR_INJECT_CTRL 0 [get_hw_sio_links $links]
  #commit_hw_sio [get_hw_sio_links $links]
  
  refresh_hw_sio $links
  puts "Finishing resetting all links"
}

#----------------------------------------------------------
#  main program
#----------------------------------------------------------
#open HW manager and connect to KCU105
#TODO: probably need to cofigure these properties for different machines?
open_hw
connect_hw_server -url localhost:3121 
current_hw_target [get_hw_targets */xilinx_tcf/Digilent/210308AB0E6E]
set_property PARAM.FREQUENCY 15000000 [get_hw_targets */xilinx_tcf/Digilent/210308AB0E6E]
open_hw_target


#program board
set_property PROGRAM.FILE {ibert_fw.bit} [get_hw_devices xcku040_0]
set_property PROBES.FILE {ibert_fw.ltx} [get_hw_devices xcku040_0]
set_property FULL_PROBES.FILE {ibert_fw.ltx} [get_hw_devices xcku040_0]
current_hw_device [get_hw_devices xcku040_0]
program_hw_devices [get_hw_devices xcku040_0]
refresh_hw_device [lindex [get_hw_devices xcku040_0] 0]


#establish links
#firmware-specific: create links based on how loopbacks are connected
set all_txs [get_hw_sio_txs]
set all_rxs [get_hw_sio_rxs]

set links [list]
set seu_counters [list]
set prev_link_status [list]

set link [create_hw_sio_link [lindex $all_txs 1] [lindex $all_rxs 2]]
lappend links [lindex [get_hw_sio_links $link] 0]
lappend seu_counters 0
lappend prev_link_status 0
set link [create_hw_sio_link [lindex $all_txs 2] [lindex $all_rxs 1]]
lappend links [lindex [get_hw_sio_links $link] 0]
lappend seu_counters 0
lappend prev_link_status 0

after 500
reset_links_all $links

#main loop- check link status and communicate with controlling GUI
set continue_test "1"
while { $continue_test != "0" } {
  #print 
  puts "SEU test in progress"

  #check status on the various links
  set nlinks [llength $links]
  refresh_hw_sio $links
  for {set ilink 0} {$ilink < $nlinks} {incr ilink} {
    set link [lindex $links $ilink]
    set seu_counter [lindex $seu_counters $ilink]
    # record bit error rate, number of bits received, prbs pattern
    set rx_ber [parse_report [get_hw_sio_links $link] "RX_BER" 3]
    set link_status [parse_report [get_hw_sio_links $link] "STATUS" 3]
    set rx_bits [parse_report [get_hw_sio_links $link] "RX_RECEIVED_BIT_COUNT" 3]
    set err_count [parse_report [get_hw_sio_links $link] "LOGIC.ERRBIT_COUNT" 3]
    #set RX_pattern [parse_report [get_hw_sio_links $link] "RX_PATTERN" 4]
    
    #write log if new SEUs
    set formatted_err_count [string trimleft $err_count 0]
    if { $formatted_err_count == "" } {
      set formatted_err_count "0"
    }
    puts "DEBUG: previously $seu_counter errors, now $formatted_err_count"
    if { $formatted_err_count > $seu_counter } {
      #new SEU since last check
      puts "SEU detected"
      set comm_output_file [open $output_file_name a]
        puts $comm_output_file "log: link $ilink now has $formatted_err_count SEUs"
      close $comm_output_file
      lset seu_counters $ilink $formatted_err_count
    }

    #write log if link broken or recovered
    if { [expr $link_status == "NO"] && [expr [lindex $prev_link_status $ilink] != "NO"] } {
      #link lost since last check
      set comm_output_file [open $output_file_name a]
        puts $comm_output_file "log: link $ilink lost"
      close $comm_output_file
      set [lindex $prev_link_status $ilink] "NO"
    } elseif { [expr $link_status != "NO"] && [expr [lindex $prev_link_status $ilink] == "NO"] } {
      #link lost since last check
      set comm_output_file [open $output_file_name a]
        puts $comm_output_file "log: link $ilink recovered"
      close $comm_output_file
      set [lindex $prev_link_status $ilink] "LINKED"
    }
  }

  #check for communication from GUI
  #note: for some reason, there is a large lag between when the file is generated and when Vivado responds
  if { [file exist $input_file_name] } {
    set comm_input_file [open $input_file_name r]
    while { [gets $comm_input_file input_data] >= 0 } {
      if { $input_data == "cmd: stop" } {
        puts "Stopping test"
        set continue_test "0"
      } elseif { $input_data == "cmd: inject error" } {
        puts "Injecting error"
        set_property LOGIC.ERR_INJECT_CTRL 1 [get_hw_sio_links $links]
        commit_hw_sio [get_hw_sio_links $links]
        after 200
        set_property LOGIC.ERR_INJECT_CTRL 0 [get_hw_sio_links $links]
        commit_hw_sio [get_hw_sio_links $links]
      }
    }
    close $comm_input_file
    file delete $input_file_name
  }
  
  after 3000
  #continue to next loop after 3s
}

close_hw
