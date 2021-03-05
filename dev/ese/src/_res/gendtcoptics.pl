# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

# See jetmsg.mc for the exact format of event log messages.

print <<EOFHDR;
//---------------------------------------------------------------------
// <copyright file="EseEventSymbols.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//---------------------------------------------------------------------

namespace Microsoft.EXO.Datamining.Cosmos
{
    using System.Collections.Generic;

    /// <summary>
    /// Mapping between ESE event IDs and their respective symbols.
    /// </summary>
    internal class EseEventSymbols
    {
        /// <summary>
        /// The event mapping dictionary.
        /// </summary>
        private static Dictionary<int, string> eventMapping = new Dictionary<int, string>();

        /// <summary>
        /// Initializes static members of the <see cref="EseEventSymbols" /> class.
        /// </summary>
        static EseEventSymbols()
        {
EOFHDR


while($line = <>) {

	# Note this depends upon the MessageId always being listed in jetmsg.mc before the SymbolicName
	if ( $line =~ /^MessageId\=/ ) {
		$ulMsgId = $line;
		$ulMsgId =~ s/^MessageId\=//;
		chomp $ulMsgId;
	} elsif ( $line =~ /^SymbolicName\=/ ) {
		$szSymbol = $line;
		$szSymbol =~ s/^SymbolicName\=//;
		chomp $szSymbol;
	}

	if ( $szSymbol ) {
		#Format is:
		#            eventMapping.Add(405, "PRE_READ_PAGE_TIME_ERROR_ID");
		#
		print "            eventMapping.Add($ulMsgId, \"$szSymbol\");\n";
		$szSymbol = "";
	}
}

print <<EOFFTR;
        }

        /// <summary>
        /// Gets the ESE event symbol associated with a given ID.
        /// </summary>
        /// <param name="id">The ESE event ID to get the symbol for.</param>
        /// <returns>The ESE event symbol associated with a given ID.</returns>
        public static string GetEventSymbol(int id)
        {
            string symbol;
            if (eventMapping.TryGetValue(id, out symbol))
            {
                return symbol;
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Determines whether the ESE event ID is recognized.
        /// </summary>
        /// <param name="id">The ESE event ID.</param>
        /// <returns>
        ///   <c>true</c> if the ESE event ID is recognized; otherwise, <c>false</c>.
        /// </returns>
        public static bool IsEseEventId(int id)
        {
            return EseEventSymbols.eventMapping.ContainsKey(id);
        }

        /// <summary>
        /// Gets an enumerable collection of ESE event IDs.
        /// </summary>
        /// <returns>
        ///   An enumerable collection of ESE event IDs.
        /// </returns>
        public static IEnumerable<int> GetEseEventIds()
        {
            List<int> eseEventIds = new List<int>();

            foreach (KeyValuePair<int, string> kvp in EseEventSymbols.eventMapping)
            {
                // Only events >= 100 are real events. Others are just category identifiers.
                if (kvp.Key >= 100)
                {
                    eseEventIds.Add(kvp.Key);
                }
            }

            return eseEventIds;
        }
    }
}
EOFFTR
